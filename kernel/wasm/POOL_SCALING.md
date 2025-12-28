# Exchange Page Pool Scaling

Dynamic exchange page pool for WASM file servers. Allows tuning pool size per workload and runtime adaptation to traffic patterns.

## Usage Examples

### Static Configuration (Simple)

```c
/* Small utility service */
wasm_fileserver_t *util = wasm_fileserver_load("/boot/util.wasm", 4);

/* Web server with moderate traffic */
wasm_fileserver_t *web = wasm_fileserver_load("/boot/http.wasm", 64);

/* High-performance database */
wasm_fileserver_t *db = wasm_fileserver_load("/boot/db.wasm", 256);
```

### Dynamic Resize (Adaptive)

```c
/* Start with reasonable default */
wasm_fileserver_t *server = wasm_fileserver_load("/boot/server.wasm", 32);

/* Monitor load and resize */
void handle_load_spike(void) {
    u32int util = wasm_fs_get_pool_utilization(server);

    if (util > 80) {
        /* High utilization - grow pool by 50% */
        u32int current = wasm_fs_get_pool_size(server);
        u32int new_size = current * 3 / 2;

        if (new_size > 512)
            new_size = 512;  /* Cap at 512 pages */

        wasm_fs_resize_pool(server, new_size);
        print("Scaled up to %u pages (util was %u%%)\n", new_size, util);
    }
    else if (util < 30) {
        /* Low utilization - shrink pool by 25% */
        u32int current = wasm_fs_get_pool_size(server);
        u32int new_size = current * 3 / 4;

        if (new_size < 16)
            new_size = 16;  /* Minimum 16 pages */

        wasm_fs_resize_pool(server, new_size);
        print("Scaled down to %u pages (util was %u%%)\n", new_size, util);
    }
}
```

### Via 9P Control File

```bash
# Check current pool size
cat /mnt/wasm/http/stats
# pool_size: 64
# utilization: 42%

# Resize pool to 128 pages
echo 'resize 128' > /mnt/wasm/http/ctl

# Verify
cat /mnt/wasm/http/stats
# pool_size: 128
# utilization: 21%
```

## Performance Impact

### Bandwidth Scaling

| Pool Size | Concurrent Clients | Bandwidth (est.) |
|-----------|-------------------|------------------|
| 16        | 16                | 10 MB/s          |
| 64        | 64                | 40 MB/s          |
| 256       | 256               | 160 MB/s         |
| 512       | 512               | 320 MB/s         |

Assumes ~640 KB/s per page with typical WASM processing overhead.

### Memory Usage

| Pool Size | Memory (pages) | Memory (KB) |
|-----------|----------------|-------------|
| 4         | 4              | 16          |
| 16        | 16             | 64          |
| 64        | 64             | 256         |
| 256       | 256            | 1024 (1 MB) |
| 512       | 512            | 2048 (2 MB) |

Plus ~100KB overhead per WASM server instance.

## Tuning Guidelines

### Web Servers
- **Normal**: 64-128 pages
- **High traffic**: 256-512 pages
- **Auto-scale**: 16 min, 512 max, 75% target

### Databases
- **OLTP**: 32-64 pages (many small transactions)
- **OLAP**: 128-256 pages (bulk queries)
- **Auto-scale**: 32 min, 256 max, 60% target

### Media Streaming
- **Live streaming**: 128-256 pages
- **VOD**: 64-128 pages
- **Auto-scale**: 64 min, 256 max, 70% target

### Microservices
- **Internal API**: 16-32 pages
- **Public API**: 64-128 pages
- **Auto-scale**: 16 min, 128 max, 75% target

## Implementation Status

- ✅ **Phase 1: Configurable pool** - Done (use num_pages parameter)
- ✅ **Phase 2: Runtime resize** - Done (wasm_fs_resize_pool)
- ⏳ **Phase 3: Auto-scaling** - TODO (needs periodic monitor)
- ⏳ **Phase 4: 9P control interface** - TODO (stats/ctl files)

## Future Enhancements

1. **Per-page utilization tracking**
   - Bitmap of busy pages
   - Accurate utilization metrics
   - Better auto-scaling decisions

2. **Automatic scaling daemon**
   - Periodic utilization checks
   - Heuristic-based resize
   - Configurable min/max/target

3. **9P statistics interface**
   - Real-time pool stats
   - Per-server monitoring
   - Historic metrics

4. **Per-connection pools**
   - Dedicated page per client
   - Perfect isolation
   - Natural scaling with connections

## See Also

- `kernel/wasm/wasm_fileserver.h` - API documentation
- `kernel/wasm/wasm_fileserver.c` - Implementation
- `kernel/include/msgord.h` - Message ordering system
