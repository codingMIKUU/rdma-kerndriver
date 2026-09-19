/* SPDX-License-Identifier: (GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause */
#ifndef MLX5_SRM_REROUTE_ABI_H
#define MLX5_SRM_REROUTE_ABI_H
#include <linux/types.h>

#ifndef MLX5_SRM_ENABLE_REROUTE
#define MLX5_SRM_ENABLE_REROUTE 0
#endif
#if MLX5_SRM_ENABLE_REROUTE != 0 && MLX5_SRM_ENABLE_REROUTE != 1
#error "MLX5_SRM_ENABLE_REROUTE must be 0 or 1"
#endif
#define MLX5_SRM_REROUTE_ABI 0x52520101U
#define MLX5_SRM_REROUTE_PATHS 4U
#define MLX5_SRM_REROUTE_USERS 65536U
#define MLX5_SRM_REROUTE_FROZEN (1ULL << 63)
#define MLX5_SRM_REROUTE_MASK (MLX5_SRM_REROUTE_FROZEN - 1)
#define MLX5_SRM_CTRL_F_REROUTE (1U << 3)
#define MLX5_SRM_REROUTE_BUDGET 64U
#ifndef MLX5_SRM_REROUTE_TEST_HOOKS
#define MLX5_SRM_REROUTE_TEST_HOOKS 0
#endif

enum mlx5_srm_route_state {
    MLX5_SRM_ROUTE_ACTIVE,
    MLX5_SRM_ROUTE_SPARE,
    MLX5_SRM_ROUTE_WAIT_READY,
    MLX5_SRM_ROUTE_COPY,
    MLX5_SRM_ROUTE_DRAIN,
    MLX5_SRM_ROUTE_TARGET,
    MLX5_SRM_ROUTE_RETIRED,
    MLX5_SRM_ROUTE_FAILED,
};
/* Occupies the two reserved cachelines of the existing 512-byte ctrl slot.
 * active[]/generation are used only on group path zero.
 * posted_bytes is protected by db_owner. Completion accounting/cursors are
 * kernel-private; the old completed_bytes/physical_cons slots are retained
 * for ABI layout only and are NOT live completion or diagnostic counters. */
struct mlx5_srm_route_ctrl {
    __u64 generation;
    __u32 active[2];
    __u32 state;
    __u32 user_db;
    __u32 abi;
    __u32 reserved;
    /* Keep every-post route/gate reads away from CQ/DB counter writes. */
    __u8 route_pad[32];
    __u64 posted_bytes;
    __u64 completed_bytes; /* reserved, do not read */
    __u64 physical_cons;   /* reserved, do not read */
    __u8 pad[40];
};
/* One persistent record per logical user QP, not per recycled SQ slot.
 * Writer stores status/origin, then release-stores sequence last. */
struct mlx5_srm_migration_completion {
    __u64 sequence;
    __u32 origin_slot;
    __u32 status;
    __u32 vendor;
    __u32 reserved;
    __u64 kernel_tsc;
};

static inline __u64 mlx5_srm_rr_seq(__u64 v)
{
    return v & MLX5_SRM_REROUTE_MASK;
}
static inline __s64 mlx5_srm_rr_delta(__u64 a, __u64 b)
{
    /* Sign extend a 63-bit modular difference; live windows < 2^62. */
    return (__s64)((a - b) << 1) >> 1;
}
#endif
