# Historical CQ simplification switch

Branch `srm-before-token-bytes` is based on `d73981d` (2026-08-26), paired with
rdma-core `a9ff6e90`. These are the last pre-MVAPICH completion-watermark
snapshots. Subsequent MPI changes and the newer three-mode CQ machinery are
not imported.

Edit `drivers/infiniband/hw/mlx5/scheduler.h` and the matching
rdma-core `providers/mlx5/mlx5.h`:

```c
#define MLX5_SRM_ENABLE_CQE_SIMPLIFY 0
```

- `0` (default): compile the historical `b286657` `srm_poll_srmc_once()`:
  hardware poll, native user-CQE routing/copy, batched per-KQP SQ recycle,
  then return shared worker credit by completed WQE span.
- `1`: compile the unchanged `d73981d` function calling
  `mlx5_ib_poll_srm_progress()`; userspace polls the shared KQP watermarks.

This branch's mode 0 is native CQ delivery, NOT the later MPI branch's
software-dispatch mode 0. There is no mode 2. Both sides of the kernel/provider
interface must be compiled in the same mode; the old ABI cannot negotiate or
detect a mode mismatch. Rebuild applications against the matching historical
rdma-core rather than combining it with installed MPI-bundled libraries.

The switch does not change NUMA affinity, hot scheduling, 512-byte control
slots, queue sizes, batching or logging parameters from the August 26 base.
It does not add the later multi-node, size-split or lifecycle fixes. Mode 0
restores the CQ path only, not every August 25 parameter or feature.

For an isolated compile check using an already configured local OFED tree:

```bash
python3 tests/check_srm_kernel_compile.py
```

This compiles scheduler/cq/qp with both macro values into temporary files,
using existing `.o.cmd` records and kernel headers. It does not link/install
or load a module, and does not overwrite the live build objects. It also
does not replace a real RDMA correctness/performance test.

The sibling rdma-core `tests/test_srm_legacy_cq_toggle.py` additionally checks
that the selected polling functions match their respective historical Git
versions and tests the userspace watermark poller without RDMA hardware.
