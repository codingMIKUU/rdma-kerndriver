# DB share and CQ cycle diagnostics

Both diagnostics are disabled by default in
`drivers/infiniband/hw/mlx5/scheduler.h`:

```c
#define MLX5_SRM_ENABLE_DB_SHARE_STATS 0
#define MLX5_SRM_ENABLE_CQE_CYCLE_STATS 0
#define MLX5_SRM_DIAG_INTERVAL_MS 1000U
```

Enable the ones needed, rebuild and load via your existing safe procedure.
DB share also requires `MLX5_SRM_ENABLE_DB_SHARE_STATS=1` in the matching
rdma-core `providers/mlx5/mlx5.h`; CQ cycle statistics only need kernel changes.
As before, CQ simplification mode itself must match on both sides.

```bash
sudo dmesg -w | rg 'SRM_DB_SHARE_STATS|SRM_CQE_CYCLE_STATS'
```

`SRM_DB_SHARE_STATS` provides both DB-operation percentages and actual
doorbelled-WQE percentages; `pct_x100=2500` is 25%. Both come from coherent
per-KQP snapshots, with bounded nonblocking reads and deferred counts carried
forward. It does not use the old delayed thread-local statistics. Match both
builds: a disabled/old provider cannot be detected from zero user counters.

`SRM_CQE_CYCLE_STATS.cqe_avg_cycles` is total cycles of nonempty
`srm_poll_srmc_once()` calls divided by actual hardware CQEs. It includes
polling/decoding, publication and credit return. Empty/skipped/error calls
are reported separately; CQEs are not the number of completed WQEs.
There is no change to CQ publication frequency or completion algorithms.

Full field definitions, caveats, provider rebuild instructions and exact
validation build commands are in the sibling rdma-core repository:
`tests/README.srm-legacy-diagnostics.md`.

Kernel build validation (temporary objects, no module install/load):

```bash
python3 tests/check_srm_kernel_compile.py --diagnostics
```

Statistics disabled compile out all new hot-path hooks. Enabled diagnostics
cost ordered TSC reads/shared counter writes/periodic logs and must not be
assumed performance-neutral. TSC ticks include interruptions and are not PMU
unhalted cycles. Existing log switches and all tuning parameters are unchanged.
