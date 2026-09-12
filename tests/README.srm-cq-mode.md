# Hollow RC completion delivery switch

`drivers/infiniband/hw/mlx5/scheduler.h` contains the build-time switch:

```c
#define MLX5_SRM_ENABLE_CQE_SIMPLIFY 0
```

- **0 (default), dispatch:** the kernel polls each hardware send CQE, routes its
  logical-QP identity, physical post index, status and vendor error into a
  separate userspace software CQ. `ibv_poll_cq()` matches that event to the
  immutable signaled-WR marker and returns the original `wr_id` and opcode.
  Advancing the KQP completion watermark alone does **not** make a logical WC
  ready in this mode.
- **1, progress:** retain the previous simplified path. The kernel publishes
  KQP completion watermarks and first-error information; the provider checks
  those watermarks to synthesize logical send WCs. No software CQ or dispatch
  status array is allocated in this mode; the existing 32-byte signaled marker
  layout is retained.
- **2, direct:** the kernel copies each hardware send CQE into a separate
  64-byte native-CQE ring, installs the logical QP's UIDX and immutable full
  post identity, and publishes the owner bit last. The provider indexes the
  logical QP directly within that CQ and returns its saved signaled marker.
  It does not walk all attached QPs per event, scan the global pending-QP
  list, or drain the entire ring before returning a requested WC. The common
  single-lane event matches its next marker; dual-lane matching retains
  per-lane cursors and ordered logical-QP completion.

This switch is independent of `MLX5_SRM_ENABLE_LARGE_KERNEL_QP` and applies to
both small/large lanes when size splitting is enabled. It does not change
DB ownership, scan policy, batch limits, scheduler CPU count or KQP count.
Ordinary RC, XRC, and the hardware receive-CQ path keep their existing behavior.

## Why dispatch and direct use a separate queue

MVAPICH can share one application CQ between Hollow sends and real XRC SRQ
receives. The kernel must not copy synthetic send CQEs into that hardware CQ:
it would compete with the NIC for producer positions. The dispatch path uses
a dedicated mmap-backed queue instead, without changing the application API.
Neither mode permits two independent producers in the NIC receive-CQ buffer.
Mode 2 restores the historical **native CQE copy + UIDX lookup + owner bit**
mechanism, not that obsolete buffer overlap or its old WR-ID-slot assumptions.
It deliberately preserves current multiple-signaled and small/large-lane
semantics. Native CQEs' unused first 24 request bytes hold CPU metadata;
UIDX (offset 32), WQE counter (60), error syndrome and owner/opcode (63)
retain their hardware layout. The kernel and provider assert this layout at
compile time. Scheduler SQs do not use CQE inline-scatter payloads.

Dispatch records are 32 bytes; direct records are 64 bytes. Per-CQ shared
storage is page-rounded `128 + entry_size * (cq.cqe + 1)` bytes; the depth
must be a power of two. Dispatch and direct also
keep a 12-byte status slot per allocated signaled-marker slot and per-lane
matching cursors. These allocations are absent in progress mode.

Direct additionally allocates a sparse CQ-local UIDX index: 256 QP pointers
per populated leaf (2 KiB on x86-64), plus a power-of-two pointer directory.
QP creation grows this index, not polling. Destruction detaches its entry
under the CQ lock before freeing the QP; a delayed CQE cannot dereference a
UIDX reused on another CQ. Leaves are released on CQ destruction. A ready-QP
FIFO only contains QPs with a ready oldest marker, preserving logical order
when large/small lanes complete out of order, without polling idle QPs.

The direct kernel reader snapshots routes with a sequence counter instead
of acquiring the route lock per CQE. Existing scheduler quiescent epochs
keep retired CQ buffers/pages alive through a poll. Consecutive events to
the same destination CQ share one software-producer lock; switching the
destination releases the old lock before taking the next. Concurrent
workers remain serialized only when writing the same application CQ.

A full software CQ leaves the hardware CQE unconsumed and does not return its
SQ slots or credits early. Full 64-bit post indices and bounded modular
comparisons handle counter wrap. QP activation records lane-specific index
floors to reject completions left over from a reused logical QP ID.

## Build/deploy

Use matching updates from `srm-cq-progress-poll-cq3` in both this repository and
the custom `rdma-core`. Stop MPI jobs before replacing libraries or modules.

For an existing MVAPICH installation, rebuild/deploy the provider first:

```bash
cd /path/to/mvapich2-2.3.7
JOBS=8 contrib/hollow-rc/rebuild_rdma_core.sh
```

For RDMA-General using the original in-place `rdma-core/build` libraries,
the provider-only build is:

```bash
cd /path/to/rdma-core
cmake --build build --target mlx5 --parallel 8
```

This command does not install libraries into `/usr` or copy them into an
MPI prefix. Ensure the running application resolves the rebuilt provider
from that build; use the MPI rebuild/deployment script above for bundled
MPI installations. No `RDMA-General` source or executable rebuild is
required for this private provider/kernel capability extension.

Choose the kernel mode before building, for example:

```c
#define MLX5_SRM_ENABLE_CQE_SIMPLIFY 2
```

The repository default remains **0**. No DB stride, scheduler count,
batch limit, SQ depth, or size-split default is changed by these commits.

Then rebuild/install the kernel driver using the established local procedure:

```bash
cd /path/to/rdma-kerndriver
sudo bash kernel_make.sh
```

The latter is a module-install/reload operation; do not run it while jobs are
using the driver, or attempt to recover an already crashed kernel this way.
The provider negotiates the mode during QP initialization, so it does not
require its own matching compile-time macro or an MPI application rebuild.
After the first provider update, changing this switch requires rebuilding and
loading the kernel driver only. Existing MPI launch commands are unchanged.

At first Hollow RTR on each CQ, the provider prints
`HOLLOW_CQ cqn=... cq_mode=dispatch`, `cq_mode=progress`, or `cq_mode=direct`
once, not per WQE.
New dispatch kernels reject providers without mode negotiation. New providers
can still use old progress-only kernels by retrying an unsupported INIT mode
probe with the legacy command prefix; this is not a silent fallback to
progress when a dispatch kernel is installed. Direct mode requires an
additional capability: old two-mode providers are rejected rather than
misinterpreting 64-byte records as 32-byte events. Updated providers first
retry an unsupported INIT probe without the direct capability before trying
the legacy prefix, so existing two-mode kernels remain usable. Direct
polling currently requires CQE version 1 (the existing ConnectX setup).

## Verification and boundaries

```bash
python3 tests/test_srm_cq_dispatch.py
python3 tests/test_srm_large_direct_db.py
python3 tests/test_srm_multipeer.py
python3 tests/check_srm_kernel_compile.py --units scheduler qp cq main
```

The compile helper checks all three completion modes crossed with both size-split
modes using existing Kbuild command records and temporary objects. It does not
link/install/load a module. Matching provider ABI and completion tests reside
in `rdma-core/tests/test_srm_cq_abi.py` and
`rdma-core/tests/test_srm_cq_dispatch.py`.

Tests cover full-ring backpressure without consuming the hardware CQE,
batched destination locking, raw owner/UIDX publication, 16-bit physical and
64-bit logical/ring wrap, saved `wr_id` across slot reuse, partial poll limits,
shared send/receive CQs, lane reordering, status propagation, and QP retirement
including UIDX reuse on a different CQ.

These are compile/mock checks, not a substitute for two-node correctness,
queue-pressure, repeated-start/stop and performance tests. The supported
Hollow interface remains `ibv_poll_cq()` with immutable **signaled** WR
markers, including multiple outstanding markers and both size lanes. CQEX
and resizing a CQ carrying dispatch storage are explicitly unsupported.
The pre-existing lack of saved identities for arbitrary unsignaled-error WRs
is not fixed by this switch; an unmatched live-QP error is diagnosed without
fabricating a `wr_id`. Use the established signaled/fixed-window completion
contract rather than assuming this change adds complete unsignaled-error
verbs semantics.

## Performance comparison

Historical throughput (for example 180 Gb/s) is **not a tested guarantee**.
Mode 2 removes known mode-0 lookup/drain overhead, but adds necessary safety
for today's interfaces. Compare 0/1/2 with identical application, QP count,
message size, batch, SQ depth, signaled frequency, DB stride, scheduler CPU
affinity and logging switches. Stop all jobs before module replacement and
check the startup `cq_mode` line. First validate payload, WC counts/order,
multi-signaled and repeated cleanup; then compare steady-state throughput
over repeated trials. Do not compare one new driver to a stale bundled MPI
provider or assume a successful compile proves historical throughput.
