# SRM post-to-doorbell timing

Set `MLX5_SRM_ENABLE_WQE_TIMING` in
`drivers/infiniband/hw/mlx5/scheduler.h` and the matching srm-rdma-core
`providers/mlx5/mlx5.h` to 1, rebuild both, and reload the rebuilt kernel
only after stopping test processes. Both macros default to 0 in commits;
local experiment settings may differ.

Each scheduler reports `SRM_DB_TIMING algorithm=srm source=kernel` to dmesg
after 1,000,000 instrumented WQEs. `post_to_db_cycles / db_wqes` is the
per-WQE mean from userspace provider entry to CPU doorbell submission
(including a WC write barrier), not NIC acknowledgment. Invalid/missing
timestamps are counted separately. The user provider retains independent
post-to-polled-CQE statistics on stderr. TSCs must be synchronized across
the CPUs running those paths.

See the matching srm-rdma-core repository's `SRM_WQE_TIMING.md` for the
complete ABI, field definitions, build instructions and measurement limits.
No tracing or timestamp sidecar is active when compiled with the macro 0.
