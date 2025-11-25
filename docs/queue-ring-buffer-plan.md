# Queue Subsystem Ring Buffer Redesign Plan

## Goal

Replace the existing qio `Queue` implementation (Block chains + QLocks) with a pointer-ring-backed design that preserves the current API surface while eliminating global locks, reducing copies, and enabling per-CPU publishing.

## Motivation

- Current qio queues serialize producers/consumers with `QLock` and copy data into `Block` lists, causing lock contention and redundant copies (seen most acutely in `/dev/cons` and `/dev/kprint`).
- Pointer ring buffers (already used for `print/prbuf`) avoid these issues by letting producers publish message pointers lock-free and having consumers subscribe via cursors.
- Extending this model to all qio users would dramatically improve scalability and simplify the data path.

## High-Level Design

1. **Maintain the qio façade:** Keep the public API (`qopen`, `qread`, `qwrite`, `Queue`) so existing callers do not change. Internally, back each `Queue` with a ring buffer instead of a Block list.
2. **Per-CPU publishers:** Each CPU gets its own single-producer ring slice for publishing to the queue. A ring0 aggregator merges per-CPU slices into the logical queue stream.
3. **Subscribers & backpressure:** Replace the old reader `QLock` with per-subscriber cursors (similar to `/dev/kprint`), each tracking its own offset and blocking via `Rendez` when there’s no data. Reference-count each message so multiple subscribers can share it safely.
4. **Ring0 aggregator / work stealing:** A central “ring0” buffer allows cross-core consumption, supporting work stealing and cross-CPU backpressure regulation.
5. **9P integration:** Use the ring channels as the transport for 9P RPCs (pipes, devmnt, network), so that the file protocol semantics are preserved on top of the new queue internals.

## Migration Plan

1. **Spec the ring channel abstraction:** Define data structures and memory ordering requirements for per-CPU publisher rings, the ring0 aggregator, and subscriber cursors.
2. **Prototype alongside qio:** Implement the ring-backed queue behind the existing API, initially opting in only select paths (e.g., `/dev/cons`, pipes) while keeping the old Block-based implementation for others.
3. **Incremental rollout:** Move subsystems one by one to the new internals, validating locking/blocking semantics (flow control, `sleep/wakeup`, error paths) via testing.
4. **Deprecate old locks:** Once all users run on ring channels, remove the qio Block+QLock code.

## Risks & Mitigations

- **Scope/complexity:** Touches many subsystems. Mitigate by keeping the API stable and migrating one user at a time.
- **Semantics:** Must match existing blocking/backpressure behavior. Extensive testing and gradual rollout reduce regressions.
- **Reference management:** Bugs in refcounting could leak or double-free shared messages. Use atomic retain/release helpers and audit all paths.
- **Performance tuning:** Rings require careful cache alignment/memory barriers. Profile early prototypes under load.

## Next Steps

1. Finish secure ramdisk work (current priority).
2. Draft detailed spec for the ring-backed queue/scheduler.
3. Prototype per-CPU publisher + ring0 aggregator (with tests) behind a feature flag.
4. Migrate `/dev/cons` queues to the new internals and measure impact.
5. Iterate and expand to other qio users.
