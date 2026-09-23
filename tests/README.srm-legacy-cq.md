# Historical CQ simplification switch

## `srm-reroute-size-aware`: independent CQ with either completion mode

The derived size-aware branch supports `MLX5_SRM_ENABLE_PRIVATE_CQ=1`
with either value of `MLX5_SRM_ENABLE_CQE_SIMPLIFY`. The latter must still
match between kernel `scheduler.h` and provider `mlx5.h`.

- Simplified mode keeps the existing private-CQ progress/watermark path.
- Native mode polls the known physical KQP's CQ, then reuses the per-CQE
  user routing/copy path. It preserves native status/vendor information,
  reroute mappings, completion timestamps and physical completion credits.
- Each native poll is bounded by the DB outstanding snapshot,
  `MLX5_SRM_PRIVATE_CQ_POLL_BUDGET`, and the scheduler WC array capacity.
  CQE pointers are normalized for both 64B and 128B hardware CQ strides.
- Hardware CQ consumer slots are returned only after the distributor has
  copied all borrowed CQEs, including its final pending publish lane.
  This avoids a temporary second CQE copy without exposing borrowed storage
  to device overwrite. Empty polls do not write the consumer doorbell.
- CQ selection, user-DB discovery, latency priority and shared-CQ mode are
  unchanged. Native mode still processes each user CQE individually; it
  does not gain simplified mode's last-watermark-only publishing.

Validation commands (temporary objects/offline tests, no installation):

```bash
python3 tests/check_srm_kernel_compile.py --private-cq 1 \
  --latency-cq-priority 1 --wqe-timing --units scheduler cq --reroute 0
python3 tests/check_srm_kernel_compile.py --private-cq 1 \
  --latency-cq-priority 1 --wqe-timing --diagnostics \
  --units scheduler cq qp --reroute 1
python3 tests/test_private_cq_native.py
bash tests/run_private_cq_offline.sh
bash tests/run_latency_cq_offline.sh
bash tests/run_reroute_offline.sh
# In the sibling rdma-core repository, regression for combined DB timing:
python3 tests/test_srm_db_timing_all.py
```

The original `sudo bash kernel_make.sh` still builds, installs and reloads
the driver; stop RDMA tests before invoking it. If changing CQE_SIMPLIFY,
also rebuild the matching provider in the build directory used by General
(in this workspace: `cmake --build build --target mlx5 -j8` in rdma-core).
No build directory, install prefix, environment or install script is changed
by this compatibility update. Hardware correctness/performance requires
testing after both matching drivers are rebuilt; offline tests cannot prove it.

After loading, inspect `sudo dmesg | grep 'SRM CQ mode='`: native private
mode reports `simplify=0` and `publish=native-cqe`.

## Historical baseline notes

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
