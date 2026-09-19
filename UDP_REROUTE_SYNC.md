# Synchronous UDP in-place reroute

This branch keeps every logical IP on its existing physical XRC initiator QP.
Rerouting changes only `qpc.primary_address_path.udp_sport` with a synchronous
RTS-to-RTS firmware command; it never allocates a spare QP, copies a WQE, or
remaps a completion.

## Build-time selection

`include/uapi/rdma/mlx5-srm-reroute.h` and the matching rdma-core header use:

```c
#define MLX5_SRM_ENABLE_REROUTE 0
#define MLX5_SRM_ENABLE_UDP_INPLACE_REROUTE 1
```

The two algorithms are mutually exclusive.  Compiling UDP reroute as zero
removes its detector, drain loop, QPC command, and DB-gate checks from the
send/CQ hot paths.

UDP reroute currently requires one scheduler worker and private per-KQP send
CQs.  The existing small/large KQP split remains supported; comparisons are
made only among KQPs in the same size level.

## Correct switch sequence

1. The scheduler acquires the KQP's shared `db_owner`.
2. It release-stores `route.user_db = 0`, snapshots `db_tail`, and releases
   the owner. Both userspace and kernel DB paths recheck this gate.
3. Users may continue reserving and filling later SQ slots, but neither side
   can ring a doorbell for them.
4. The scheduler actively polls that KQP's private CQ until
   `cq_complete_idx >= frozen_db_tail`.
5. It synchronously modifies only the UDP source port with RTS-to-RTS.
6. It release-stores `route.user_db = 1`; queued WQEs can then be doorbelled on
   the new ECMP path.

A timeout or firmware error reopens the gate and leaves the original port in
use. Module shutdown also aborts the drain and reopens the gate.

## Runtime controls

All parameters are under `/sys/module/mlx5_ib/parameters/`:

```text
srm_udp_reroute_enable                default 1
srm_udp_reroute_log_enable            default 0
srm_udp_reroute_interval_ms           default 10
srm_udp_reroute_ratio_gap             default 200 (scale 0..1000)
srm_udp_reroute_consecutive_windows   default 3
srm_udp_reroute_min_wqes              default 64
srm_udp_reroute_cooldown_ms            default 1000
srm_udp_reroute_drain_timeout_ms       default 5000
```

For example:

```bash
echo 1 | sudo tee /sys/module/mlx5_ib/parameters/srm_udp_reroute_enable
echo 1 | sudo tee /sys/module/mlx5_ib/parameters/srm_udp_reroute_log_enable
echo 150 | sudo tee /sys/module/mlx5_ib/parameters/srm_udp_reroute_ratio_gap
```

`ratio_gap` is the main trigger threshold. A KQP must trail the best active
KQP in its size level by this many completion-ratio points for the configured
number of strictly consecutive windows.

## Verification

Compile-time mode and runtime settings are printed during module/scheduler
initialization. Events can be followed with:

```bash
sudo dmesg -w | grep -E 'SRM reroute mode=udp|SRM UDP (detect|reroute)'
```

Successful switching produces, in order, `event=detect`, `event=freeze`, and
`event=complete`. `event=failed` includes timeout or firmware command errors.
Per-window output is emitted only when `srm_udp_reroute_log_enable=1`.

The completion event reports three exact timing fields:

```text
software_cycles + hardware_cycles = total_cycles
```

`hardware_cycles` brackets only the synchronous `mlx5_cmd_exec()` firmware
command. `total_cycles` starts when the successful reroute attempt enters the
freeze path and ends after the DB gate is reopened. `software_cycles` is the
remaining time, including owner/gate handling, old-path CQ drain, command
preparation and post-command publication.
