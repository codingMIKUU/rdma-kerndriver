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

This switch is independent of `MLX5_SRM_ENABLE_LARGE_KERNEL_QP` and applies to
both small/large lanes when size splitting is enabled. It does not change
DB ownership, scan policy, batch limits, scheduler CPU count or KQP count.
Ordinary RC, XRC, and the hardware receive-CQ path keep their existing behavior.

## Why dispatch uses a separate queue

MVAPICH can share one application CQ between Hollow sends and real XRC SRQ
receives. The kernel must not copy synthetic send CQEs into that hardware CQ:
it would compete with the NIC for producer positions. The dispatch path uses
a dedicated mmap-backed queue instead, without changing the application API.
It is a safe per-CQE dispatch implementation, not a byte-for-byte restoration
of the obsolete hardware-buffer-copy path.

Each record is 32 bytes. Per-CQ shared storage is page-rounded
`128 + 32 * (cq.cqe + 1)` bytes; the depth must be a power of two. Dispatch also
keeps a 12-byte status slot per allocated signaled-marker slot and per-lane
matching cursors. These allocations are absent in progress mode.

A full software CQ leaves the hardware CQE unconsumed and does not return its
SQ slots or credits early. Full 64-bit post indices and bounded modular
comparisons handle counter wrap. QP activation records lane-specific index
floors to reject completions left over from a reused logical QP ID.

## Build/deploy

Use matching updates from `srm-cq-progress-poll` in both this repository and
the custom `rdma-core`. Stop MPI jobs before replacing libraries or modules.

For an existing MVAPICH installation, rebuild/deploy the provider first:

```bash
cd /path/to/mvapich2-2.3.7
JOBS=8 contrib/hollow-rc/rebuild_rdma_core.sh
```

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
`HOLLOW_CQ cqn=... cq_mode=dispatch` or `cq_mode=progress` once, not per WQE.
New dispatch kernels reject providers without mode negotiation. New providers
can still use old progress-only kernels by retrying an unsupported INIT mode
probe with the legacy command prefix; this is not a silent fallback to
progress when a dispatch kernel is installed.

## Verification and boundaries

```bash
python3 tests/test_srm_cq_dispatch.py
python3 tests/test_srm_large_direct_db.py
python3 tests/test_srm_multipeer.py
python3 tests/check_srm_kernel_compile.py --units scheduler qp cq main
```

The compile helper checks both completion modes crossed with both size-split
modes using existing Kbuild command records and temporary objects. It does not
link/install/load a module. Matching provider ABI and completion tests reside
in `rdma-core/tests/test_srm_cq_abi.py` and
`rdma-core/tests/test_srm_cq_dispatch.py`.

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
