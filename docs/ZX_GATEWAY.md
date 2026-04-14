# ZX, NSD, and Ordered Control Plane

## Purpose

This document defines the intended full-stack role of `ZX`, `nsd`, and the
ordered control plane in Lux9.

The design goal is not "replace 9P with ZX". The goal is:

- keep `9P` as the local kernel and userspace service ABI
- use `ZX` for wide-area traversal, streaming, and remote namespace access
- keep `nsd` as the namespace assembler and policy surface
- use `msgord` and higher-level consensus only for ordered control state
- keep bulk data and mutable service semantics out of the namespace layer

That split preserves the current kernel shape while making the distributed
namespace stronger and more mobile.

## Core Position

The stack should be:

- local kernel/service boundary: `9P`
- local kernel ordering substrate: `msgord`
- cluster control plane: userspace ordered/consensus services
- wide-area remote access: `ZX`
- namespace assembly: `nsd`
- service supervision and restart: `resurrection`
- mutable state: ordinary `9P` services
- bulk immutable data: content-addressed stores or specialized file servers

The kernel should not parse or route `ZX`. The kernel remains a local message
router that understands `9P`, exchange transport, and local ordering.

## Why This Split

`9P` is already the natural local ABI in Lux9:

- the kernel router already parses and dispatches `Fcall`
- local services already publish routed endpoints
- exchange pages and doorbell/ring paths already carry local traffic

`ZX` is a better fit for:

- remote traversal over slower links
- streaming reads and writes
- cross-site namespace access
- roaming session mounts
- higher-latency WAN scenarios where plain local 9P assumptions are weak

`nsd` is the right place to assemble a process or node namespace, but not the
right place to store bulk data or implement distributed consensus itself.

## Control Plane vs Data Plane

This design only works if the split is strict.

Control plane:

- namespace intents
- mount and bind decisions
- service publication and withdrawal
- service ownership and failover
- session leases and session placement
- mutable metadata head updates for shared state
- capability and lease records

Data plane:

- file contents
- large blobs
- media streams
- artifacts
- package trees
- cache payloads
- UI drawing traffic

Consensus and ordered messaging belong only in the control plane.

## Layer Responsibilities

### Kernel

The kernel owns:

- local `9P` routing
- exchange-page transport
- local endpoint dispatch
- local `msgord` ordering for conflict-declared operations
- capability enforcement
- IRQ, scheduling, and process isolation

The kernel does not own:

- cluster-wide agreement
- WAN protocol logic
- remote traversal policy
- namespace policy
- bulk storage

### `msgord`

`msgord` is the local ordered submission and admission substrate.

It should provide:

- explicit conflict-declared ordering
- barrier semantics
- parent-closed local delivery
- deterministic ordering for local service operations

It is not, by itself, the cluster consensus layer.

For cluster-wide agreement, higher-level userspace services can build on the
same conflict-declared model, but the distributed protocol stays outside the
kernel.

### `nsd`

`nsd` owns namespace assembly.

It should decide:

- what is mounted
- where a service appears in the namespace
- which mounts are local vs remote
- which namespace intents are node-local vs cluster-shared

`nsd` should not:

- carry bulk data
- implement remote transport directly
- invent its own storage layer
- perform consensus internally

Instead, `nsd` consumes ordered namespace intents from the control plane and
materializes the corresponding local mount graph.

### `zxproxy`

Add a userspace daemon, `zxproxy`, with two sides:

- local side: a normal `9P` endpoint that `nsd` can mount
- remote side: a `ZX` client session to a remote site, service, or namespace

`zxproxy` owns:

- 9P to ZX translation
- remote session setup
- reconnect and retry policy
- WAN-facing caching
- remote handle and path bookkeeping

This keeps all WAN complexity out of the kernel.

### `resurrection`

`resurrection` remains the Layer 2 service supervisor.

It should manage:

- `zxproxy` instances
- local control-plane daemons
- critical mutable-state services
- restart and recovery policy for distributed-facing services

This matters because namespace mobility is only useful if the services behind
it can be restarted and reattached reliably.

### Ordered or Consensus Services

Cluster-wide agreement belongs in userspace services, not the kernel.

That layer should own:

- ordered namespace intents
- service registry state
- failover decisions
- session lease changes
- shared metadata updates

This service can expose a `9P` control surface such as:

- `/consensus/<domain>/submit`
- `/consensus/<domain>/wait`
- `/consensus/<domain>/events`
- `/consensus/<domain>/barrier`
- `/consensus/<domain>/members`

The important point is that `nsd` consumes the result of this ordered layer; it
does not replace it.

## Local Node Shape

On a single node, the stack should look like:

1. local app issues `9P` operations
2. kernel routes the `Fcall`
3. `msgord` orders conflict-sensitive local operations where needed
4. local services handle the request
5. `nsd` assembles the namespace from local and remote endpoints
6. `resurrection` supervises the service set

If a path is remote-backed:

1. `nsd` has mounted a local `zxproxy`
2. app still sees a normal local `9P` service
3. `zxproxy` translates to remote `ZX`
4. reply comes back as a normal `9P` reply

The kernel never sees `ZX`.

## Cluster Shape

Across multiple nodes, the architecture should be:

- each node runs a local `9P` service fabric
- each node runs `nsd`
- selected nodes run ordered/consensus services
- each node may run one or more `zxproxy` instances for remote access
- `resurrection` supervises local control-plane and service daemons

Cluster consistency comes from ordered namespace and service intents, not from
trying to globally serialize all file traffic.

## Namespace Model

There should be two namespace classes:

- local namespace
- cluster namespace

Local namespace:

- process-private binds
- node-private mounts
- temporary local policy
- no cluster consensus required

Cluster namespace:

- shared service mount points
- replicated service names
- roaming session roots
- distributed device or service ownership
- failover-sensitive paths

Only the cluster namespace should flow through the ordered or consensus layer.

## Mutable State Model

Mutable state should be served by ordinary `9P` servers, not by `nsd`.

Examples:

- `homed` for mutable home metadata
- `sessiond` for session and desktop state
- `leased` for leases and locks
- service-specific metadata daemons for object heads and version pointers

These mutable services can map state to content-addressed data underneath, but
their semantics remain ordinary service semantics, not namespace semantics.

This keeps `nsd` small and keeps mutability in the services that actually own
the data model.

## Bulk Data Model

Bulk data should sit behind file or object services.

Examples:

- content-addressed blob stores
- IPFS-like immutable object stores
- dedicated file servers
- cache servers
- artifact stores

The recommended pattern is:

1. mutable `9P` service updates metadata or a head pointer
2. underlying blobs are stored in a content-addressed system
3. `nsd` mounts the service into the namespace
4. `ZX` is used when that service is remote

This gives a filesystem-shaped interface without forcing `nsd` to become a
storage engine.

## Roaming Session Model

This is one of the main reasons to do the full stack.

Target flow:

1. user authenticates at a new edge node
2. ordered control plane commits a session lease move
3. `nsd` on the new node materializes the user's namespace
4. local draw/input devices are mounted on the new node
5. remote home or session services are mounted locally via `zxproxy`
6. `rio` starts against the recreated namespace
7. session state is resumed or resurrected by userspace services

This gives roaming session resurrection.

It does not require:

- full live process migration
- global shared memory
- consensus on every file read or write

The namespace and the control plane move first. Process continuity is a
separate problem.

## Grid and Global Namespace

With `nsd` tied to ordered namespace intents and `ZX` providing remote access,
the "grid" becomes stronger than simple service discovery.

The cluster can expose:

- stable shared service paths
- deterministic service failover
- replayable namespace state
- convergent distributed mount decisions
- location-independent user namespaces

That is the intended direction for a global filesystem-shaped operating
environment.

The key idea is:

- users log into the namespace
- the namespace is assembled locally
- remote services are mounted through `zxproxy`
- ordered control state decides what the namespace should be

## Translation Model

`zxproxy` keeps local 9P state and remote ZX state.

Core mapping:

- local `fid` -> remote ZX handle/session/path state

Typical flow:

1. local client sends `Tattach`, `Twalk`, `Topen`, `Tread`, `Twrite`, and so on
2. `zxproxy` resolves the corresponding `ZX` operation
3. remote response is translated back into a local `9P` reply
4. local client still sees a normal `9P` service

For remote-backed mutable services, the application still talks to a normal
service path. `zxproxy` only changes transport and latency behavior.

## Ordered Namespace Intents

The ordered or consensus layer should carry compact control records such as:

- `mount path=/u/alice/home endpoint=homed@site-b`
- `bind path=/srv/db endpoint=db@site-c`
- `lease session=alice owner=edge-17`
- `failover service=mail old=node-a new=node-d`
- `head service=homed user=alice cid=<new-root>`

Those records are what `nsd` replays into its namespace graph.

The ordered layer should not carry:

- bulk directory trees
- entire files
- window draw traffic
- large artifact payloads

## Security and Recovery

This stack only makes sense if service recovery is designed in.

Required properties:

- `resurrection` can restart critical control-plane services
- ordered namespace state can be replayed after node restart
- `nsd` can reconstruct cluster-visible mounts from committed state
- `zxproxy` can reconnect and recover remote sessions cleanly
- capability and lease state can be revalidated after restart

That is the minimum needed for roaming sessions and distributed failover to be
credible.

## Implementation Plan

### Phase 1: Gateway Foundation

1. Add `userspace/zxproxy/` as a local `9P` server and remote `ZX` client.
2. Mount `zxproxy` from `nsd` using the normal endpoint path.
3. Start with read-only remote mounts.

### Phase 2: Writable Remote Services

1. Add streamed writes.
2. Add reconnect and retry policy.
3. Add local caching rules for WAN-backed services.

### Phase 3: Namespace Integration

1. Distinguish local and cluster namespace intents in `nsd`.
2. Teach `nsd` to materialize remote-backed mounts from ordered state.
3. Add support for roaming user namespace reconstruction.

### Phase 4: Ordered Control Plane

1. Expose a userspace ordered or consensus service over `9P`.
2. Move shared namespace intents into that service.
3. Move session lease and failover records into that service.
4. Keep `msgord` as the local ordering substrate underneath.

### Phase 5: Service Recovery Integration

1. Make `resurrection` supervise `zxproxy` and control-plane daemons.
2. Make node restart replay ordered namespace state.
3. Reattach or resurrect sessions on a new edge node.

## Non-Goals

Do not:

- replace local kernel `9P` routing with `ZX`
- move WAN protocol handling into the kernel
- make `nsd` a storage engine
- treat consensus as the bulk data path
- require cluster consensus for process-private local binds
- claim live process migration as part of the initial design

## Summary

The intended architecture is:

- `9P` for local kernel and service interaction
- `ZX` for remote and WAN-facing access
- `nsd` for namespace assembly
- `msgord` for local ordered control of conflicting operations
- userspace ordered or consensus services for cluster-shared control state
- ordinary `9P` services for mutable state
- separate content or file services for bulk data

If this is implemented cleanly, Lux9 gets:

- a stronger grid model
- stable distributed namespaces
- roaming sessions
- service mobility and failover
- a clear separation of control plane and data plane

That is the full scope of the design.
