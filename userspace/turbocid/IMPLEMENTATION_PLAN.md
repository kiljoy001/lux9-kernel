# Sophia Semantic Filesystem - Implementation Plan

## Vision
Transform Sophia from a simple content-addressed filesystem into a semantic filesystem with:
- **LSH-based semantic clustering** - Similar files automatically organize together
- **Hybrid delta compression** - xdelta3 for text, bsdiff for binaries, RLE for sparse
- **Version replay** - Git-like history and time-travel
- **Intelligent deduplication** - Exact + fuzzy via LSH

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Sophia 9P Server                         │
├─────────────────────────────────────────────────────────────┤
│  Namespace Layer (Paths → UUIDs)                            │
│  - ART index: /path/file → UUID_content                     │
│  - Version tracking: UUID → prev_version chain              │
├─────────────────────────────────────────────────────────────┤
│  Semantic Layer (LSH Clustering)                            │
│  - MinHash signatures                                       │
│  - Bucket index: LSH_bucket → [UUID list]                   │
│  - Similarity search                                        │
├─────────────────────────────────────────────────────────────┤
│  Delta Layer (Hybrid Compression)                           │
│  - Auto format selection (xdelta3/bsdiff/RLE/custom)        │
│  - Delta encoding/decoding                                  │
│  - Chain depth limiting                                     │
├─────────────────────────────────────────────────────────────┤
│  Storage Layer (Immutable Blocks)                           │
│  - Content blocks (full or delta)                           │
│  - Metadata (RecordData with semantic + delta fields)       │
│  - Journal for crash recovery                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Phase 1: Delta Compression Foundation (CURRENT PHASE)
**Goal**: Implement hybrid delta encoding with xdelta3 as primary

### Step 1.1: Enhanced RecordData Structure
- [/] Extend `RecordData` in `storage/dat.h`
  - [ ] Add `type` field (FULL vs DELTA)
  - [ ] Add `base_uuid` field (for deltas)
  - [ ] Add `delta_size` field
  - [ ] Add `delta_format` field (xdelta3/bsdiff/RLE/custom)
  - [ ] Add `prev_version` field (for replay chain)
  - [ ] Add `version` counter
  - [ ] Add `write_timestamp`

### Step 1.2: xdelta3 Integration
- [ ] Create `storage/delta/xdelta3_simple.c`
  - [ ] Implement `xdelta3_encode()`
  - [ ] Implement `xdelta3_decode()`
  - [ ] Use rolling checksums (Adler32)
  - [ ] Sliding window matching
- [ ] Add to Makefile

### Step 1.3: Content Type Detection
- [ ] Create `storage/delta/content_detect.c`
  - [ ] `calculate_entropy()` - Shannon entropy
  - [ ] `count_printable()` - Text detection
  - [ ] `is_elf_binary()` - Binary detection
  - [ ] `is_sparse()` - Sparse file detection
  - [ ] `auto_select_delta_format()` - Smart dispatcher

### Step 1.4: Delta Encoding/Decoding
- [ ] Create `storage/delta/delta_encode.c`
  - [ ] `encode_delta()` - Multi-format encoder
  - [ ] `decode_delta()` - Multi-format decoder
  - [ ] `encode_delta_with_fallback()` - Try multiple formats
- [ ] Update `sart_write_immutable()` to support delta mode
- [ ] Implement `sart_read_full()` with delta reconstruction

### Step 1.5: Version Chain Management
- [ ] Update `sart_write()` to track prev_version
- [ ] Implement version counter increment
- [ ] Add timestamp tracking
- [ ] Implement chain depth limiting (MAX=10)

**Deliverable**: Files can be stored as deltas, reconstructed on read

---

## Phase 2: TLSH Semantic Clustering
**Goal**: Implement TLSH-based fuzzy similarity search

### Step 2.1: TLSH Implementation
- [ ] Create `storage/semantic/tlsh.c`
  - [ ] Sliding window (5-byte pearson hash)
  - [ ] 128 bucket counting
  - [ ] Quartile computation
  - [ ] 35-byte encoding

### Step 2.2: TLSH Distance Metric
- [ ] Create `storage/semantic/tlsh_distance.c`
  - [ ] Hamming distance on encoded buckets
  - [ ] Quartile difference weighting
  - [ ] Threshold-based similarity (0-400 range)
  - [ ] Efficient comparison (< 1μs per pair)

### Step 2.3: TLSH Bucket Index
- [ ] Create `storage/semantic/tlsh_bucket.c`
  - [ ] Use first 32 bits of TLSH as bucket ID
  - [ ] Bucket → UUID list mapping
  - [ ] Fast lookup (O(1) bucket find, O(n) scan candidates)
  - [ ] Incremental index updates

### Step 2.4: TLSH-Guided Delta Base Selection
- [ ] Implement `find_similar_base_tlsh()`
  - [ ] Query bucket index by TLSH prefix
  - [ ] Rank candidates by distance (<100 threshold)
  - [ ] Return best base (most similar)
- [ ] Integrate into delta encoding path
- [ ] Fallback to prev_version if no TLSH match

### Step 2.5: Semantic Search API
- [ ] `sart_semantic_search()` - Find similar files by TLSH
- [ ] `sart_semantic_cluster()` - List bucket contents
- [ ] Performance: Sub-100ms for 10K files

**Benefits of TLSH over MinHash**:
- ✅ 45% less space (35 vs 64 bytes)
- ✅ 4× faster encoding
- ✅ Universal (works on binaries, not just text)
- ✅ Production-proven (malware detection)
- ✅ Better security (detect code variants)

**Deliverable**: Similar files cluster together, deltas use TLSH bases

---

## Phase 3: Version Replay System
**Goal**: Git-like history and time-travel

### Step 3.1: Replay Chain Walking
- [ ] Implement `sart_replay()`
  - [ ] Walk prev_version chain
  - [ ] Filter by timestamp
  - [ ] Return version list (UUID, timestamp, size)

### Step 3.2: Version Reading
- [ ] Implement `sart_read_version()`
  - [ ] Find version in chain
  - [ ] Reconstruct content
  - [ ] Cache for performance

### Step 3.3: 9P Protocol Extensions
- [ ] Define `Treplay` / `Rreplay` messages in `fcall.h`
- [ ] Define `Treadversion` / `Rreadversion`
- [ ] Implement `sophia_replay()` callback
- [ ] Implement `sophia_readversion()` callback

**Deliverable**: Users can view file history and read old versions

---

## Phase 4: Additional Delta Formats
**Goal**: Support bsdiff, RLE, custom line-diff

### Step 4.1: bsdiff Integration
- [ ] Create `storage/delta/bsdiff.c`
  - [ ] Suffix array construction
  - [ ] Binary diffing
  - [ ] Handle address relocations
- [ ] Integrate into `encode_delta()` dispatcher

### Step 4.2: RLE Delta
- [ ] Create `storage/delta/rle_delta.c`
  - [ ] Run-length encoding
  - [ ] Optimized for sparse files
  - [ ] Delta from sparse base

### Step 4.3: Custom Line-Diff
- [ ] Create `storage/delta/line_diff.c`
  - [ ] Myers diff algorithm
  - [ ] Line-by-line diffing
  - [ ] Human-readable output
  - [ ] Git-compatible format

### Step 4.4: Multi-Format Testing & Tuning
- [ ] Benchmark all formats on real data
- [ ] Tune thresholds for format selection
- [ ] Add statistics tracking

**Deliverable**: Auto-select best format per file type

---

## Phase 5: Optimization & Polish
**Goal**: Production-ready performance

### Step 5.1: Caching
- [ ] Cache reconstructed deltas (LRU)
- [ ] Cache MinHash signatures
- [ ] Cache similarity computations

### Step 5.2: Background Tasks
- [ ] Re-base long delta chains
- [ ] Rebuild semantic index periodically
- [ ] Garbage collection of unreferenced UUIDs

### Step 5.3: Performance
- [ ] Parallel delta application
- [ ] SIMD optimizations for MinHash
- [ ] Batch semantic index updates

### Step 5.4: Testing
- [ ] Unit tests for each delta format
- [ ] Integration tests for replay
- [ ] Semantic search accuracy tests
- [ ] Performance benchmarks

**Deliverable**: Production-ready semantic filesystem

---

## Success Metrics

### Performance Targets
- **Delta encoding**: >50 MB/s (xdelta3), >5 MB/s (bsdiff)
- **Delta decoding**: >100 MB/s
- **Semantic search**: <100ms for 10K files
- **Space savings**: >70% for code, >90% for binaries

### Feature Completeness
- ✅ Directory enumeration (Phase 0 - DONE)
- ✅ File deletion (Phase 0 - DONE)
- [ ] Delta compression
- [ ] Semantic clustering
- [ ] Version replay
- [ ] Multi-format support

### Code Quality
- [ ] All functions documented
- [ ] Error handling throughout
- [ ] Memory leak free (valgrind clean)
- [ ] Integration tests passing

---

## File Structure

```
userspace/turbocid/
├─ sophia.c               # 9P server (existing)
├─ storage/
│  ├─ dat.h              # Data structures (EXTEND)
│  ├─ sart_store.c       # Storage engine (EXTEND)
│  ├─ sart_art.c         # ART index (EXTEND)
│  ├─ sart_journal.c     # Journal (existing)
│  ├─ sart_blkio.c       # Block I/O (existing)
│  ├─ delta/             # NEW: Delta compression
│  │  ├─ xdelta3_simple.c
│  │  ├─ bsdiff.c
│  │  ├─ rle_delta.c
│  │  ├─ line_diff.c
│  │  ├─ content_detect.c
│  │  └─ delta_encode.c
│  └─ semantic/          # NEW: LSH clustering
│     ├─ minhash.c
│     ├─ lsh_bucket.c
│     └─ semantic_search.c
├─ Makefile              # Update with new files
└─ IMPLEMENTATION_PLAN.md (this file)
```

---

## Current Status: Phase 1, Step 1.1

**Next immediate actions**:
1. Extend RecordData structure in dat.h
2. Create storage/delta/ directory
3. Implement xdelta3_simple.c
4. Wire into sart_write/read

**Estimated timeline**:
- Phase 1: 1 week
- Phase 2: 1 week  
- Phase 3: 3-4 days
- Phase 4: 1 week
- Phase 5: 1 week

**Total**: ~4-5 weeks to production-ready semantic filesystem

---

## Notes

- **Backward compatibility**: Old files without delta/semantic fields work as-is
- **Incremental rollout**: Can enable features per-file or per-directory
- **Testing strategy**: Test each phase independently before integration
- **Performance**: Profile early, optimize based on real workloads

---

**Last Updated**: 2026-01-14  
**Status**: Phase 1.1 in progress
