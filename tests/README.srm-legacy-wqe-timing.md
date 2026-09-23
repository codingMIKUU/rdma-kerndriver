# WQE timing and amortized CQE P99

Enable `MLX5_SRM_ENABLE_WQE_TIMING=1` in `scheduler.h` and the matching
rdma-core `providers/mlx5/mlx5.h`, then rebuild both updated drivers.
`SRM_DB_TIMING source=all` now includes user and kernel DB, printed in dmesg
per worker at `MLX5_SRM_DIAG_INTERVAL_MS` (1000 ms) when there was activity.
`user_post_to_db_avg_cycles` and `kernel_post_to_db_avg_cycles` are the two
source averages; `post_to_db_avg_cycles` is the sum of their cycles divided
by the sum of their valid WQE counts, not the mean of the two averages.
All times end at DB helper return, not at hardware completion.

Actual WQE counts (`*_db_wqes`) and valid denominators (`*_timed_wqes`)
are separate. Missing/invalid timestamps are exposed and excluded. A busy
KQP snapshot defers both sources without losing their samples. User DB off
naturally reduces the combined metric to kernel-only timing. DB share and
CQE cycle switches are independent; they need not be on for this report.

The shared control slot stays 512 bytes; timing reuses 24 bytes of reserved
DB-share storage. Bit 5 advertises the new combined-timing capability.
The provider snapshots timestamps during its existing scan before ringing,
then records one end TSC per successful batch under the existing owner.
Migrated requests retain the original post time; synthetic NOPs are excluded.
Disabling WQE timing in both drivers compiles out the additional hooks.

Post-to-completion timing remains in stderr as `SRM_WQE_TIMING`, after each
1,000,000 observed WQEs per thread, independent of the DB reporting interval.

`MLX5_SRM_ENABLE_CQE_CYCLE_STATS` now also prints
`cqe_batch_avg_p99_cycles_upper`, with `p99_weight=cqes`. It is a CQE-weighted
P99 of poll-batch amortized cycles, not individually timed CQE P99. It uses
a bounded logarithmic histogram and reports the selected bucket's upper bound.
The existing mean and one-second reporting window are unchanged.

For the precise boundaries, storage cost, timing perturbation, clock/counter
limitations, enable/disable and build commands see the sibling rdma-core file
`tests/README.srm-legacy-wqe-timing.md`.

```bash
python3 tests/check_srm_kernel_compile.py --wqe-timing --diagnostics --units scheduler cq qp
```

Only temporary objects are compiled; nothing is installed or loaded.
