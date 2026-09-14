# WQE timing and amortized CQE P99

`MLX5_SRM_ENABLE_WQE_TIMING` in `scheduler.h` defaults to 0. Enable the
matching macro in rdma-core `providers/mlx5/mlx5.h` too, then rebuild both.
Kernel DB timing is printed as `SRM_DB_TIMING source=kernel` after each
1,000,000 sampled WQEs per worker. Post-to-completion timing is in stderr as
`SRM_WQE_TIMING`; user DB is deliberately not timed.

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
