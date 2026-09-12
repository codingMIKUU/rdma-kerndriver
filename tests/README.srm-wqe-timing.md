# QPSwitch post-to-doorbell timing

The matching compile-time switch is `MLX5_SRM_ENABLE_WQE_TIMING` in
`drivers/infiniband/hw/mlx5/scheduler.h` and rdma-core's
`providers/mlx5/mlx5.h`. Default `0` preserves the existing SQ metadata size
and compiles timing out of the kernel send/poll hot paths. Rebuild both with
`1` to collect timing; the provider rejects a mismatched new kernel/provider.

Each physical SQ publish mmap then appends 16-byte CPU-only timestamp records.
The user writes `{post_tsc, sequence}` before publishing the WQE's ready token.
The scheduler captures the timestamps for exactly the batch it will send before
ringing DB, then samples ordered TSC immediately after the DB helper returns.
It aggregates after releasing `db_owner`. This works with either small/large
QP layout, multiple workers, and both dispatched/software-CQE and watermark
completion modes. User direct DB performs the equivalent measurement in the
provider; kernel CQ processing never rereads a DB timestamp after slot reuse.

`sudo dmesg -w` shows, per scheduler worker and every 1,000,000 checked WQEs:

```text
SRM_DB_TIMING algorithm=qpswitch source=kernel sched=0 worker=0 db_calls=... db_wqes=... post_to_db_cycles=... post_to_db_avg_cycles=... missing_timestamps=... invalid_timestamps=...
```

`db_wqes` counts valid per-WQE samples, `post_to_db_cycles` sums their latencies,
and the average is integer division of those two numbers. Missing generations
and impossible negative TSC differences are excluded and counted separately.
The endpoint is CPU-side doorbell submission, not NIC acknowledgement. Enabling
timing perturbs performance; synchronized TSCs across user/kernel CPUs are
required. No forced CQ signaling, reservation limit, DB batching or payload
format changes are made. The final partial interval is not emitted.

The provider emits its user-DB timing and original post-to-polled-CQE timing to
application stderr. Its `tests/README.srm-wqe-timing.md` documents aggregation,
unsignaled-WQE semantics and CPU-only wrap/reuse regression tests. Reinstalling
or reloading modules is deliberately outside these tests.
