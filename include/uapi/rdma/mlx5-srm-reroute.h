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
#define MLX5_SRM_REROUTE_ABI 0x52520103U
/* Per physical KQP: actual DB payload not yet completed by hardware.
 * 0 disables this experimental cap. Published by the kernel at path init,
 * so the provider uses the loaded kernel's value, not its own build default.
 * An oversized WR may be DB'd alone when this physical QP has no in-flight
 * payload. Drain-source kernel DB bypasses the cap but keeps accounting. */
#ifndef MLX5_SRM_MAX_INFLIGHT_BYTES
#define MLX5_SRM_MAX_INFLIGHT_BYTES (20480ULL)
#endif
#if MLX5_SRM_MAX_INFLIGHT_BYTES < 0 || MLX5_SRM_MAX_INFLIGHT_BYTES > 0xffffffffULL
#error "MLX5_SRM_MAX_INFLIGHT_BYTES must fit in u32 (0 disables the cap)"
#endif
#define MLX5_SRM_REROUTE_PATHS 4U
#define MLX5_SRM_REROUTE_USERS 65536U
#define MLX5_SRM_REROUTE_FROZEN (1ULL << 63)
#define MLX5_SRM_REROUTE_MASK (MLX5_SRM_REROUTE_FROZEN - 1)
#define MLX5_SRM_CTRL_F_REROUTE (1U << 3)
/* Immutable provider/kernel capability, also present without rerouting. */
#define MLX5_SRM_CTRL_F_BYTE_WINDOW (1U << 4)
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
 * posted_bytes is protected by db_owner. With a nonzero byte cap, the sole
 * CQ worker publishes completed_bytes (physical, NOT logical completions).
 * Byte counters are cumulative modulo 2^64; reuse must not reset them.
 * physical_cons remains reserved; physical CQ cursors stay kernel-private. */
struct mlx5_srm_route_ctrl {
    __u64 generation;
    __u32 active[2];
    __u32 state;
    __u32 user_db;
    __u32 abi;
    __u32 inflight_limit_bytes;
    /* Keep every-post route/gate reads away from CQ/DB counter writes. */
    __u8 route_pad[32];
    __u64 posted_bytes;
    __u64 completed_bytes; /* live when inflight_limit_bytes != 0 */
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
/* Caller holds db_owner. A stale CQ snapshot is conservative; no atomic
 * fetch-add is necessary because user/kernel DB writers use the same lock.
 * Unsigned subtraction also handles cumulative byte-counter wrap. */
static inline __u64 mlx5_srm_rr_byte_available(__u64 posted, __u64 completed,
                                               __u32 limit)
{
    __u64 outstanding = posted - completed;
    if (!limit) return ~0ULL;
    return outstanding < limit ? limit - outstanding : 0;
}
static inline int mlx5_srm_rr_byte_fits(__u64 available, __u64 prefix,
                                        __u64 payload)
{
    return prefix <= available && payload <= available - prefix;
}
/* Oversized admission is a singleton exception, not a larger batch budget.
 * available == limit certifies zero outstanding payload under db_owner.
 * Track the WQE count too: a zero-byte prefix must not permit extra WRs.
 * Unlimited/drain snapshots (~0ULL) take the ordinary fit path. */
static inline int mlx5_srm_rr_byte_can_post(__u64 available, __u64 prefix,
        __u64 payload, __u32 limit, __u32 batch_wqes)
{
    return mlx5_srm_rr_byte_fits(available, prefix, payload) ||
        (limit && payload > limit && !batch_wqes && !prefix &&
         available == limit);
}
/* Shared SQ uses XRC descriptors. Read payload lengths, not remote addresses
 * or inline data; the descriptor stays immutable until cons_idx is released.
 * No extra per-slot metadata/cacheline writes are needed. */
static inline __u32 mlx5_srm_payload_word(const void *wqe, unsigned int off)
{
    __u32 word;
    __builtin_memcpy(&word, (const char *)wqe + off, sizeof(word));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    word = __builtin_bswap32(word);
#endif
    return word;
}
static inline __u64 mlx5_srm_payload_bytes(const void *wqe)
{
    unsigned int op = mlx5_srm_payload_word(wqe, 0) & 0xff;
    unsigned int end = (mlx5_srm_payload_word(wqe, 4) & 0x3f) * 16;
    unsigned int off;
    __u64 bytes = 0;
    switch (op) {
    case 0x08: case 0x09: case 0x10: /* XRC WRITE, WRITE_IMM, READ */
        off = 48; break;
    case 0x01: case 0x0a: case 0x0b: /* XRC SEND variants */
        off = 32; break;
    case 0x11: case 0x12: case 0x14: case 0x15: /* atomic variants */
        off = 64; break;
    default: return 0; /* e.g. maintenance NOP, no application payload */
    }
    while (off + 4 <= end) {
        __u32 count = mlx5_srm_payload_word(wqe, off);
        if (count & 0x80000000U) {
            bytes += count & 0x7fffffffU;
            break; /* inline segment is the last payload segment */
        }
        if (off + 16 > end) break;
        bytes += count;
        off += 16;
    }
    return bytes;
}
#endif
