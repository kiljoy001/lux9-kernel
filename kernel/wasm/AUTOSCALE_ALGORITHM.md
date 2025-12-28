# WASM Pool Auto-Scaling Algorithm

## Algorithm: Hybrid AIMD + EMA with Hysteresis

Combines proven techniques from TCP congestion control, Linux load averaging, and control theory.

## Core Components

### 1. **EMA (Exponential Moving Average) Smoothing**

Filters out transient spikes to avoid over-reacting to noise.

```
smoothed_util = α × current_util + (1-α) × smoothed_util

where α = 0.3 (ema_alpha = 30)
```

**Why EMA?**
- Used in Linux load averages for decades
- Gives more weight to recent samples, but remembers history
- Filters 1-2 second transient spikes
- Tracks trends over ~3-5 seconds

**Example:**
```
Time: 0s   1s   2s   3s   4s   5s
Real: 20%  95%  90%  85%  30%  25%
EMA:  20%  42%  57%  66%  55%  46%

Without EMA: Would trigger growth at 2s, shrink at 4s (oscillation!)
With EMA:    Grows at 3s when trend confirmed, stays stable (correct!)
```

### 2. **AIMD (Additive Increase, Multiplicative Decrease)**

Inspired by TCP congestion control.

**Growth (Multiplicative):**
```
new_size = current_size × 1.5  (grow by 50%)
```
- Fast reaction to load spikes
- Aggressive enough to handle bursts
- Scales proportionally with pool size

**Shrink (Multiplicative but slower):**
```
new_size = current_size × 0.75  (shrink by 25%)
```
- Conservative shrinking avoids thrashing
- Asymmetric: grow faster than shrink
- Prevents oscillation

**Why AIMD?**
- Proven at Internet scale (TCP uses it)
- Self-stabilizing
- Converges to optimal size
- Fair under competition

**Example:**
```
Pool evolution during traffic spike:

Time  Load   Action         Pool Size
0s    60%    (stable)       64 pages
1s    92%    Grow +50%      96 pages
2s    88%    Grow +50%      144 pages
3s    75%    (deadband)     144 pages  ← Stabilized
4s    72%    (deadband)     144 pages
5s    30%    (wait 3s)      144 pages
8s    28%    Shrink -25%    108 pages
11s   25%    Shrink -25%    81 pages   ← Gradual return
```

### 3. **Hysteresis (Different Thresholds)**

Prevents oscillation around a single threshold.

```
Grow threshold:   85% utilization
Shrink threshold: 50% utilization
Deadband:         50-85% (no action)
```

**Why Hysteresis?**
- 35% gap prevents ping-pong between grow/shrink
- Once grown, won't immediately shrink
- Provides stable steady state
- Classic control theory technique

**Example without hysteresis:**
```
Single threshold at 75%:
  74% → shrink → 80% → grow → 73% → shrink → ...
  OSCILLATION! (wastes CPU, destabilizes)
```

**Example with hysteresis:**
```
Thresholds at 50% and 85%:
  74% → no action (in deadband)
  73% → no action (stable)
  86% → grow
  72% → no action (hysteresis gap)
  48% → shrink (only if sustained)
  STABLE!
```

### 4. **Stability Counter (Anti-Thrash)**

Requires sustained low load before shrinking.

```c
if (smoothed_util < low_threshold) {
    if (stable_count >= 3) {
        shrink();
        stable_count = 0;
    } else {
        stable_count++;
    }
}
```

**Why?**
- Prevents shrinking on temporary lulls
- Requires 3 consecutive low intervals (~3 seconds)
- Growth is immediate (react fast to load)
- Shrink is delayed (avoid thrashing)

**Example:**
```
Traffic pattern: busy → quiet 2s → busy again

Without stability counter:
  0s: 80% → stable
  1s: 40% → shrink to 48 pages
  2s: 85% → grow to 72 pages  ← Wasted work!

With stability counter:
  0s: 80% → stable
  1s: 40% → count=1, wait
  2s: 85% → count reset, grow if needed ← Correct!
```

## Complete Algorithm Flow

```
Every check_interval (default 1000ms):

1. Measure current_util = wasm_fs_get_pool_utilization(server)

2. Update EMA:
   smoothed_util = (α × current + (100-α) × smoothed) / 100

3. Decision tree:

   if smoothed_util > high_threshold (85%):
       # Overloaded: grow immediately
       new_size = current_size × 1.5
       new_size = min(new_size, max_pages)
       resize(new_size)
       stable_count = 0

   elif smoothed_util < low_threshold (50%):
       # Underutilized: wait for confirmation
       if stable_count >= 3:
           new_size = current_size × 0.75
           new_size = max(new_size, min_pages)
           resize(new_size)
           stable_count = 0
       else:
           stable_count++

   else:
       # In deadband (50-85%): no action
       stable_count++
       stable_count = min(stable_count, 10)  # Cap to prevent overflow
```

## Tuning Parameters

### Default Configuration

```c
WasmAutoScaleConfig defaults = {
    .min_pages = current_size / 2,    // Don't shrink below half
    .max_pages = current_size * 8,    // Don't grow beyond 8x
    .target_util = 75,                // Aim for 75% utilization
    .high_threshold = 85,             // Grow above 85%
    .low_threshold = 50,              // Shrink below 50%
    .check_interval = 1000,           // Check every 1 second
    .ema_alpha = 30,                  // EMA smoothing factor (0.3)
};
```

### Tuning for Different Workloads

**Bursty traffic (e.g., web server):**
```c
.high_threshold = 80,     // Grow sooner
.low_threshold = 40,      // Shrink more aggressively
.ema_alpha = 40,          // React faster (less smoothing)
.check_interval = 500,    // Check more frequently
```

**Steady traffic (e.g., database):**
```c
.high_threshold = 90,     // Tolerate higher utilization
.low_threshold = 60,      // Keep more headroom
.ema_alpha = 20,          // Smoother (ignore transients)
.check_interval = 2000,   // Check less frequently
```

**Memory-constrained (e.g., embedded):**
```c
.max_pages = 64,          // Hard cap on memory
.low_threshold = 30,      // Aggressive shrinking
```

## Performance Characteristics

### Reaction Time

**To load spike:**
```
Traffic spike at t=0:
  t=0.0s: 95% load detected
  t=0.0s: EMA = 0.3×95 + 0.7×50 = 63%
  t=1.0s: EMA = 0.3×95 + 0.7×63 = 72%
  t=2.0s: EMA = 0.3×95 + 0.7×72 = 79%
  t=3.0s: EMA = 0.3×95 + 0.7×79 = 84%
  t=4.0s: EMA = 0.3×95 + 0.7×84 = 87% → GROW!

Reaction time: 4-5 seconds (filters transients)
```

**To load drop:**
```
Traffic drop at t=0:
  t=0.0s: 40% load detected
  t=1.0s: EMA crosses 50% threshold, count=1
  t=2.0s: Still low, count=2
  t=3.0s: Still low, count=3 → SHRINK!

Reaction time: 3 seconds (requires confirmation)
```

### Stability

**Oscillation frequency:**
- Theoretically: Cannot oscillate faster than check_interval
- Practically: Hysteresis prevents oscillation entirely
- Worst case: One grow-shrink cycle per 10 seconds (if pathological)

**Steady state:**
- Pool size stabilizes within 50-85% utilization range
- No adjustments in deadband → zero overhead
- Typical: 99% of time spent stable (1% adjusting)

### Overhead

**CPU cost per tick:**
```
1. Measure utilization:      ~100 CPU cycles
2. EMA calculation:           ~50 cycles
3. Comparison:                ~20 cycles
4. Resize (if needed):        ~10,000 cycles (malloc/free)

Total: 170 cycles (stable) or 10,170 cycles (resizing)
```

At 1 check/second:
- Stable: 0.000017% CPU (negligible)
- Resizing: 0.001% CPU (still negligible)

**Memory overhead:**
```c
sizeof(WasmAutoScaleConfig) = ~48 bytes per server

100 WASM servers = 4.8 KB (trivial)
```

## Comparison to Alternatives

| Algorithm | Reaction Time | Stability | Complexity |
|-----------|---------------|-----------|------------|
| **AIMD+EMA+Hysteresis** | 3-5s | Excellent | Medium |
| Simple threshold | <1s | Poor (oscillates) | Low |
| PID controller | 1-2s | Good | High |
| Token bucket | Variable | Good | Medium |
| Exponential backoff | 10-20s | Excellent | Low |

**Why not PID?**
- Requires tuning 3 parameters (Kp, Ki, Kd)
- Can overshoot or undershoot
- Overkill for this problem
- AIMD is simpler and proven

**Why not simple threshold?**
- Oscillates badly around threshold
- No smoothing of transients
- Unstable under variable load

**Why not token bucket?**
- Doesn't prevent oscillation
- Just rate-limits resizes (doesn't decide when)
- Could combine with our algorithm

## Testing Scenarios

### Scenario 1: Flash Crowd

```
Traffic pattern: 10 → 1000 clients instantly

Expected behavior:
  t=0s:  10 clients, 16 pages (62% util)
  t=1s:  1000 clients, still 16 pages (saturated!)
  t=4s:  EMA crosses 85%, grow to 24 pages
  t=5s:  Still high, grow to 36 pages
  t=6s:  Still high, grow to 54 pages
  t=7s:  Still high, grow to 81 pages
  ...
  t=15s: Stabilizes at ~400 pages (75% util)

Growth rate: Exponential until reaching capacity
Time to stabilize: ~15 seconds
```

### Scenario 2: Gradual Ramp

```
Traffic pattern: 10 → 100 clients over 60 seconds

Expected behavior:
  Gradual growth tracking load
  No oscillation
  Settles at optimal size (~32 pages)
  Smooth, predictable
```

### Scenario 3: Oscillating Load

```
Traffic pattern: 20 ↔ 80 clients, period 10s

Expected behavior:
  EMA smooths oscillations
  Pool size stays relatively stable (~24-32 pages)
  Doesn't track every spike
  Correct behavior (avoid thrashing)
```

## See Also

- `wasm_fileserver.h` - API documentation
- `wasm_fileserver.c` - Implementation
- `POOL_SCALING.md` - Usage examples
- TCP congestion control (RFC 5681)
- Linux load averages (kernel/sched/loadavg.c)
