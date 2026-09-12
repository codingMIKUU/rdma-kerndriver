#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t u64;
typedef int64_t s64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef u64 __u64;
typedef u32 __u32;
typedef u8 __u8;
#define U64_MAX UINT64_MAX
#define U32_MAX UINT32_MAX
#define MLX5_SRM_PUBLISH_SEQ_MASK ((1ULL << 48) - 1)
#define MLX5_DEVICE_STATE_INTERNAL_ERROR 1
#define MLX5_CQE_REQ 0
#define MLX5_CQE_REQ_ERR 13
#define MLX5_CQE_RESIZE_CQ 5
#define IB_WC_SUCCESS 0
#define IB_WC_LOC_PROT_ERR 4
#define IB_WC_WR_FLUSH_ERR 5
#define READ_ONCE(x) (x)
#define WRITE_ONCE(x, v) ((x) = (v))
#define smp_load_acquire(p) (*(p))
#define smp_store_release(p, v) (*(p) = (v))
#define unlikely(x) (x)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define pr_err_ratelimited(...) ((void)0)
#define pr_warn_ratelimited(...) ((void)0)
#define rmb() ((void)0)
#define spin_lock_irqsave(lock, flags) ((void)(lock), (flags) = 0)
#define spin_unlock_irqrestore(lock, flags) ((void)(lock), (void)(flags))
#define spin_lock(lock) ((void)(lock))
#define spin_unlock(lock) ((void)(lock))
#define mutex_lock(lock) ((void)(lock))
#define mutex_unlock(lock) ((void)(lock))
#define be32_to_cpu(x) be32toh(x)
#define be16_to_cpu(x) be16toh(x)
#define kfree(p) free(p)

struct mlx5_srm_sw_cq;
struct mlx5_ib_cqbuf {
    struct mlx5_srm_sw_cq *sw_buf;
    u32 sw_depth;
    u64 sw_producer;
    int cqn, sw_lock;
};
struct mlx5_ib_usr_rc_route {
    struct mlx5_ib_cqbuf *cqb;
    bool dispatch_ready;
    int dispatch_lock;
    u32 small_kqp_idx, large_kqp_idx;
    u64 small_post_floor, large_post_floor;
};
struct mlx5_ib_sched_group {
    struct mlx5_ib_usr_rc_route usr_rc_routes[8];
    int cq_lock;
} sched_group;
struct mlx5_sq_ctrl_page {
    u64 resv_idx, cons_idx, completion_error_idx;
    u32 completion_error_status, completion_error_vendor;
};
struct mlx5_ib_srmc {
    u32 srmc_idx;
    u64 cq_complete_idx;
    struct mlx5_sq_ctrl_page *ctrl_page;
    u64 tokens[8];
};
struct mlx5_ib_wq { u32 wqe_cnt, tail; };
struct mlx5_core_qp { u32 qp_num; };
struct mlx5_ib_qp {
    struct mlx5_core_qp ibqp;
    bool is_srmc_kernel_qp;
    struct mlx5_ib_srmc *srmc_owner;
    struct mlx5_ib_wq sq;
};
struct mlx5_core_dev { int state; };
struct mlx5_ib_dev {
    struct mlx5_core_dev *mdev;
    struct { struct mlx5_ib_qp *tree[2]; } qp_table;
};
struct ib_cq { struct mlx5_ib_dev *device; };
struct mlx5_cqe64 {
    u32 sop_drop_qpn;
    u16 wqe_counter;
    u8 opcode;
    u32 status, vendor;
};
struct mlx5_err_cqe { struct mlx5_cqe64 cqe; };
struct ib_wc { u32 status, vendor_err; };
struct mlx5_core_cq { u32 cons_index, cqe_sz, reported_ci; };
struct mlx5_ib_cq {
    struct ib_cq ibcq;
    struct mlx5_core_cq mcq;
    struct mlx5_cqe64 hw[8];
    u32 hw_producer;
    int lock, buf, *resize_buf;
};

static struct mlx5_ib_dev *to_mdev(struct mlx5_ib_dev *dev) { return dev; }
static struct mlx5_ib_cq *to_mcq(struct ib_cq *cq) { return (void *)cq; }
static struct mlx5_ib_qp *to_mibqp(struct mlx5_core_qp *qp) { return (void *)qp; }
static void free_cq_buf(struct mlx5_ib_dev *dev, int *buf) { }
static void mlx5_cq_set_ci(struct mlx5_core_cq *cq) { cq->reported_ci = cq->cons_index; }
static void *next_cqe_sw(struct mlx5_ib_cq *cq)
{
    return cq->mcq.cons_index == cq->hw_producer ? NULL :
           &cq->hw[cq->mcq.cons_index & 7];
}
static u8 get_cqe_opcode(struct mlx5_cqe64 *cqe) { return cqe->opcode; }
static struct mlx5_core_qp *radix_tree_lookup(struct mlx5_ib_qp *(*tree)[2], u32 qpn)
{
    for (unsigned i = 0; i < 2; i++)
        if ((*tree)[i] && (*tree)[i]->ibqp.qp_num == qpn)
            return &(*tree)[i]->ibqp;
    return NULL;
}
static void mlx5_handle_error_cqe(struct mlx5_ib_dev *dev,
                                struct mlx5_err_cqe *err, struct ib_wc *wc)
{
    wc->status = err->cqe.status;
    wc->vendor_err = err->cqe.vendor;
}
static u64 mlx5_ib_srmc_get_publish_token(struct mlx5_ib_srmc *srmc, u32 idx)
{
    return srmc->tokens[idx & 7];
}
static u64 mlx5_srm_publish_seq(u64 token) { return token >> 16; }
static u16 mlx5_srm_publish_usr_rc(u64 token) { return token; }

#include "actual_dispatch.h"

_Static_assert(sizeof(struct mlx5_srm_sw_cqe) == 32, "event ABI");
_Static_assert(sizeof(struct mlx5_srm_sw_cq) == 128, "ring ABI");

static void enqueue(struct mlx5_ib_cq *cq, struct mlx5_ib_qp *qp, u64 post,
                    u32 usr_rc, u32 status, u32 vendor)
{
    struct mlx5_cqe64 *hw = &cq->hw[cq->hw_producer++ & 7];

    *hw = (struct mlx5_cqe64){ .sop_drop_qpn = htobe32(qp->ibqp.qp_num),
        .wqe_counter = htobe16(post), .opcode = status ? MLX5_CQE_REQ_ERR : MLX5_CQE_REQ,
        .status = status, .vendor = vendor };
    qp->srmc_owner->tokens[post & 7] =
        (((post + 1) & MLX5_SRM_PUBLISH_SEQ_MASK) << 16) | usr_rc;
}

int main(void)
{
    struct mlx5_core_dev core = {0};
    struct mlx5_ib_dev dev = { .mdev = &core };
    struct mlx5_sq_ctrl_page ctrl[2] = {
        { .completion_error_idx = U64_MAX }, { .completion_error_idx = U64_MAX }
    };
    struct mlx5_ib_srmc owners[2] = {
        { .srmc_idx = 7, .ctrl_page = &ctrl[0] },
        { .srmc_idx = 71, .ctrl_page = &ctrl[1] }
    };
    struct mlx5_ib_qp qps[2] = {
        { .ibqp.qp_num = 100, .is_srmc_kernel_qp = true,
          .srmc_owner = &owners[0], .sq.wqe_cnt = 8 },
        { .ibqp.qp_num = 200, .is_srmc_kernel_qp = true,
          .srmc_owner = &owners[1], .sq.wqe_cnt = 8 }
    };
    struct mlx5_ib_cq cq = { .ibcq.device = &dev, .mcq.cqe_sz = 64 };
    struct mlx5_ib_cqbuf user = { .sw_depth = 2, .cqn = 17 };
    struct mlx5_srm_sw_cq *sw;
    u32 completed;

    dev.qp_table.tree[0] = &qps[0];
    dev.qp_table.tree[1] = &qps[1];
    sw = calloc(1, sizeof(*sw) + 2 * sizeof(sw->entries[0]));
    assert(sw);
    user.sw_buf = sw;
    sched_group.usr_rc_routes[3].cqb = &user;
    sched_group.usr_rc_routes[4].cqb = &user;
    assert(!mlx5_ib_activate_srm_cq_route(&sched_group, 3, &owners[0], &owners[1]));
    assert(!mlx5_ib_activate_srm_cq_route(&sched_group, 4, &owners[0], &owners[1]));

    /* One signaled CQE releases its preceding unsignaled WQEs as credits. */
    enqueue(&cq, &qps[0], 2, 3, 0, 0);
    enqueue(&cq, &qps[1], 1, 4, 0, 0);
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == 2);
    assert(completed == 5 && sw->producer == 2 && cq.mcq.reported_ci == 2);
    assert(ctrl[0].cons_idx == 3 && ctrl[1].cons_idx == 2);
    assert(sw->entries[0].post_idx == 2 && sw->entries[0].kqp_idx == 7);
    assert(sw->entries[1].post_idx == 1 && sw->entries[1].kqp_idx == 71);
    assert(sw->entries[0].usr_rc == 3 && sw->entries[1].usr_rc == 4);

    enqueue(&cq, &qps[0], 4, 3, 0, 0);
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == 0);
    assert(completed == 0 && cq.mcq.cons_index == 2 && cq.mcq.reported_ci == 2);
    assert(ctrl[0].cons_idx == 3 && owners[0].cq_complete_idx == 3);
    assert(sw->entries[0].post_idx == 2); /* Full ring was not overwritten. */
    sw->consumer = 1;
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == 1);
    assert(completed == 2 && ctrl[0].cons_idx == 5 && sw->producer == 3);
    assert(sw->entries[0].post_idx == 4);

    /* Invalid userspace consumer must never make an occupied slot writable. */
    enqueue(&cq, &qps[0], 5, 3, 0, 0);
    sw->consumer = 4;
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == -EIO);
    assert(completed == 0 && cq.mcq.cons_index == 3 && sw->producer == 3);
    sw->consumer = 3;
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == 1);

    /* A removed route drains without pinning shared SQ credits forever. */
    sched_group.usr_rc_routes[3].cqb = NULL;
    enqueue(&cq, &qps[0], 6, 3, 0, 0);
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == 1);
    assert(completed == 1 && sw->producer == 4 && ctrl[0].cons_idx == 7);
    sched_group.usr_rc_routes[3].cqb = &user;

    /* Reused IDs before RTR must not stall orphan CQEs on a missing ring. */
    sched_group.usr_rc_routes[3].dispatch_ready = false;
    user.sw_buf = NULL;
    owners[0].tokens[7] = (8ULL << 16) | 3;
    assert(!mlx5_ib_srm_dispatch_completion(&owners[0], 7, 0, 0));
    assert(sw->producer == 4);
    user.sw_buf = sw;
    ctrl[0].resv_idx = 8;
    ctrl[1].resv_idx = 3;
    assert(!mlx5_ib_activate_srm_cq_route(&sched_group, 3, &owners[0], &owners[1]));
    assert(sched_group.usr_rc_routes[3].small_post_floor == 8);
    assert(sched_group.usr_rc_routes[3].large_post_floor == 3);
    /* RTR records the reservation floors, not the hardware completion tail. */
    assert(!mlx5_ib_srm_dispatch_completion(&owners[0], 7, 0, 0));
    owners[1].tokens[2] = (3ULL << 16) | 3;
    assert(!mlx5_ib_srm_dispatch_completion(&owners[1], 2, 0, 0));
    {
        struct mlx5_ib_srmc wrong_lane = owners[0];
        wrong_lane.srmc_idx = 99;
        wrong_lane.tokens[0] = (9ULL << 16) | 3;
        assert(!mlx5_ib_srm_dispatch_completion(&wrong_lane, 8, 0, 0));
    }
    assert(sw->producer == 4);
    ctrl[0].resv_idx = 100;
    assert(!mlx5_ib_activate_srm_cq_route(&sched_group, 3, &owners[0], &owners[1]));
    assert(sched_group.usr_rc_routes[3].small_post_floor == 8);

    /* Physical 16-bit wrap extends separately for each KQP; errors survive. */
    sw->consumer = sw->producer;
    owners[0].cq_complete_idx = 65535;
    ctrl[0].cons_idx = 65535;
    enqueue(&cq, &qps[0], 65536, 3, IB_WC_LOC_PROT_ERR, 0x77);
    enqueue(&cq, &qps[0], 65537, 3, IB_WC_WR_FLUSH_ERR, 0x55);
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == 2);
    assert(completed == 3 && ctrl[0].cons_idx == 65538);
    assert(sw->entries[0].post_idx == 65536);
    assert(sw->entries[0].status == IB_WC_LOC_PROT_ERR);
    assert(sw->entries[0].vendor_err == 0x77);
    assert(sw->entries[1].status == IB_WC_WR_FLUSH_ERR);
    assert(ctrl[0].completion_error_idx == 65537);
    assert(ctrl[0].completion_error_status == IB_WC_LOC_PROT_ERR);
    assert(ctrl[0].completion_error_vendor == 0x77);

    /* A live route missing its registered ring cannot report false success. */
    user.sw_buf = NULL;
    enqueue(&cq, &qps[1], 2, 4, 0, 0);
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == -EINVAL);
    assert(completed == 0 && ctrl[1].cons_idx == 2);
    user.sw_buf = sw;
    sw->consumer = sw->producer;
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == 1);

    /* Dispatch never copied into the hardware receive CQ or its consumer. */
    assert(cq.mcq.cons_index == 8 && cq.mcq.reported_ci == 8);

    /* Software producer/consumer arithmetic also survives u64 wrap. */
    user.sw_producer = UINT64_MAX - 1;
    sw->producer = sw->consumer = user.sw_producer;
    enqueue(&cq, &qps[0], 65538, 3, 0, 0);
    enqueue(&cq, &qps[0], 65539, 3, 0, 0);
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == 2);
    assert(completed == 2 && sw->producer == 0 && user.sw_producer == 0);
    enqueue(&cq, &qps[0], 65540, 3, 0, 0);
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == 0);
    assert(completed == 0 && ctrl[0].cons_idx == 65540);
    sw->consumer = 0;
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == 1);
    assert(sw->producer == 1 && ctrl[0].cons_idx == 65541);

    /* Stale hardware counters cannot be extended into a false next epoch. */
    enqueue(&cq, &qps[0], 65540, 3, 0, 0);
    assert(mlx5_ib_poll_srm_dispatch(&cq.ibcq, 8, &completed) == -EIO);
    assert(completed == 0 && ctrl[0].cons_idx == 65541);
    free(sw);
    puts("PASS: actual kernel CQ dispatch, backpressure, errors, wrap and lanes");
    return 0;
}
