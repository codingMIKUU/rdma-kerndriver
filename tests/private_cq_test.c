/* Mock hardware around the production private-CQ poll function. The runner
 * appends the marked function from cq.c; no second copy of its algorithm. */
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <limits.h>
#include <rdma/mlx5-srm-reroute.h>

typedef unsigned long long u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
static inline s64 mlx5_srm_seq_delta(u64 a, u64 b)
{
#if MLX5_SRM_ENABLE_REROUTE
    return (s64)((a - b) << 1) >> 1;
#else
    return (s64)(a - b);
#endif
}
#define likely(x) (x)
#define unlikely(x) (x)
#define min_t(t,a,b) ((t)(a) < (t)(b) ? (t)(a) : (t)(b))
#define READ_ONCE(x) (x)
#define smp_load_acquire(p) (*(p))
#define WRITE_ONCE(x,v) ((x) = (v))
#define be32_to_cpu(x) ntohl(x)
#define be16_to_cpu(x) ntohs(x)
#define spin_lock_irqsave(lock,flags) do { (void)(lock); (flags)=0; } while (0)
#define spin_unlock_irqrestore(lock,flags) do { (void)(lock); (void)(flags); } while (0)
#define rmb() ((void)0)
#define wmb() ((void)0)
#define pr_warn_ratelimited(...) ((void)0)
#define pr_err_ratelimited(...) ((void)0)
#define U64_MAX UINT64_MAX
#define MLX5_DEVICE_STATE_INTERNAL_ERROR 9
#define MLX5_CQE_REQ 0
#define MLX5_CQE_REQ_ERR 13
#define MLX5_CQE_RESIZE_CQ 5
#define IB_WC_SUCCESS 0
#define kfree free

struct mlx5_cqe64 {
    u32 sop_drop_qpn;
    u16 wqe_counter;
    u8 opcode;
    u8 padding[57];
};
_Static_assert(sizeof(struct mlx5_cqe64) == 64, "mock CQE size");
struct mlx5_err_cqe { int unused; };
struct ib_wc { u32 status, vendor_err; };
struct mlx5_core_dev { int state; };
struct mlx5_ib_dev { struct mlx5_core_dev *mdev; };
struct ib_cq { struct mlx5_ib_dev *device; };
struct mlx5_ib_cq_buf { int unused; };
struct mock_mcq { u32 cons_index, cqe_sz; };
struct mlx5_ib_cq {
    struct ib_cq ibcq;
    struct mock_mcq mcq;
    int lock;
    struct mlx5_ib_cq_buf buf, *resize_buf;
};
struct mlx5_sq_ctrl_page {
    u64 cons_idx, completion_error_idx, db_tail;
    u32 completion_error_status, completion_error_vendor;
    struct mlx5_srm_route_ctrl route;
};
struct mlx5_ib_srmc;
struct mock_fbc { unsigned char *buf; };
struct mlx5_ib_wq { u32 tail, wqe_cnt; struct mock_fbc fbc; };
struct mlx5_ib_qp {
    struct { u32 qp_num; } ibqp;
    struct mlx5_ib_wq sq;
    struct mlx5_ib_srmc *srmc_owner;
};
struct mlx5_ib_srmc {
    struct { struct mlx5_ib_qp *qp; struct ib_cq *cq; } ini_cb;
    struct mlx5_sq_ctrl_page *ctrl_page;
    u64 cq_complete_idx;
    int srmc_idx;
    int sig_cnt;
    u64 byte_completed;
};
struct mlx5_ib_sched_worker { int unused; };
struct mlx5_ib_sched { struct mlx5_ib_sched_worker *worker; };
static struct mlx5_ib_sched_worker *mlx5_srm_cq_worker(
    struct mlx5_ib_sched *sched, struct mlx5_ib_srmc *srmc)
{ (void)srmc; return sched->worker; }
static __always_inline int mlx5_srm_refresh_poll_budget(
    struct mlx5_ib_sched *sched, struct mlx5_ib_srmc *srmc);

static struct mlx5_core_dev core;
static struct mlx5_ib_dev dev = { &core };
static struct mlx5_ib_cq cq;
static struct mlx5_sq_ctrl_page ctrl;
static struct mlx5_ib_qp qp;
static struct mlx5_ib_srmc s;
static unsigned char entries[2048][128];
static unsigned char sq_entries[1024][64];
static u32 byte_publishes;
#if !MLX5_SRM_ENABLE_REROUTE && MLX5_SRM_MAX_INFLIGHT_BYTES
static void *mlx5_frag_buf_get_wqe(struct mock_fbc *f, u32 index)
{ return f->buf + index * 64; }
static u64 rr_db_byte_available(struct mlx5_ib_srmc *);
static u64 rr_db_prefix_bytes(struct mlx5_ib_srmc *, u64, u32, u32, u64);
void mlx5_srm_complete_byte_span(struct mlx5_ib_srmc *, u64, u32);
#endif
static u32 head, ci_stores, publishes, timing_calls, last_ci;
static u64 last_time_post;
#if MLX5_SRM_ENABLE_REROUTE
static int rr_idle, rr_maintenance, rr_failures, rr_slow_calls;
static int rr_publish_pending;
static u32 rr_batch_calls, rr_batch_cqes;
static u64 rr_maintenance_end;
static struct mlx5_ib_srmc *rr_mapped_origin;
static struct mlx5_sq_ctrl_page rr_origin_ctrl;
static struct mlx5_ib_srmc rr_origin_s;
#endif

#define to_mcq(p) ((struct mlx5_ib_cq *)(p))
#define to_mdev(p) (p)
static void *next_cqe_sw(struct mlx5_ib_cq *c)
{
    return c->mcq.cons_index == head ? NULL : entries[c->mcq.cons_index];
}
static u8 get_cqe_opcode(struct mlx5_cqe64 *e) { return e->opcode; }
static void mlx5_cq_set_ci(struct mock_mcq *c)
{
    ++ci_stores;
    last_ci = c->cons_index;
}
static void publish(u64 *p, u64 val)
{
    if (p == &ctrl.route.completed_bytes) {
        ++byte_publishes; *p = val; return;
    }
    assert(ci_stores && last_ci == cq.mcq.cons_index);
    ++publishes;
    *p = val;
}
#define smp_store_release(p,v) publish(p,v)
static void free_cq_buf(struct mlx5_ib_dev *d, struct mlx5_ib_cq_buf *b)
{ (void)d; (void)b; }
static void mlx5_handle_error_cqe(struct mlx5_ib_dev *d,
                                 struct mlx5_err_cqe *e, struct ib_wc *wc)
{ (void)d; (void)e; wc->status=13; wc->vendor_err=0x87; }
#if MLX5_SRM_ENABLE_WQE_TIMING
static u64 rdtsc_ordered(void) { return 1234; }
static void mlx5_srm_timing_publish_kernel_cqe(struct mlx5_ib_srmc *p,
                                              u64 post, u64 tsc)
{ assert(p == &s && tsc == 1234); ++timing_calls; last_time_post=post; }
#endif

#if MLX5_SRM_ENABLE_REROUTE
static int mlx5_srm_rr_maintenance_post(struct mlx5_ib_srmc *p,
                                         u16 ctr, u64 *post)
{
    if (p == &s && rr_maintenance && ctr == (u16)(rr_maintenance_end-1)) {
        *post=mlx5_srm_rr_seq(rr_maintenance_end-1);
        return 1;
    }
    return 0;
}
static void mlx5_srm_rr_fail(struct mlx5_ib_srmc *p)
{ assert(p==&s); ++rr_failures; }
static int mlx5_srm_rr_idle(const struct mlx5_ib_srmc *p)
{ assert(p==&s); return rr_idle; }
static void mlx5_srm_rr_complete_idle_batch(struct mlx5_ib_srmc *p,
                                             u64 start, u32 count)
{
    assert(p==&s && count);
    ++rr_batch_calls;
    rr_batch_cqes+=count;
    assert(start==ctrl.cons_idx || rr_batch_calls==1);
    rr_publish_pending=1;
    (void)start;
}
static int mlx5_srm_rr_complete(struct mlx5_ib_srmc **physical, u64 *post,
                                 u32 status, u32 vendor, u64 tsc)
{
    struct mlx5_ib_srmc *origin;
    assert(*physical==&s && vendor==(status ? 0x87U : 0U));
    (void)tsc;
    ++rr_slow_calls;
    if (rr_maintenance && mlx5_srm_rr_seq(*post+1)==rr_maintenance_end) {
        rr_maintenance=0;
        return 2;
    }
    rr_publish_pending=1;
    origin=rr_mapped_origin ? rr_mapped_origin : &s;
    if (status) {
        origin->ctrl_page->completion_error_idx=mlx5_srm_rr_seq(*post+1);
        origin->ctrl_page->completion_error_status=status;
    }
    *physical=origin;
    return origin!=&s;
}
struct mlx5_ib_sched_group { struct mlx5_ib_sched scheds[1]; };
static struct mlx5_ib_sched_group sched_group;
static void mlx5_srm_rr_flush_native(struct mlx5_ib_sched *sched)
{
    assert(sched==&sched_group.scheds[0]);
    if (rr_publish_pending) {
        if (rr_mapped_origin)
            publish(&rr_mapped_origin->ctrl_page->cons_idx,
                    rr_mapped_origin->cq_complete_idx+1);
        else
            publish(&ctrl.cons_idx,s.cq_complete_idx);
        rr_publish_pending=0;
    }
}
#endif

static inline u64 mlx5_ib_srmc_complete_post(struct mlx5_ib_qp *,
                                               struct mlx5_ib_wq *, u16, u32 *);
int mlx5_ib_poll_srm_private_progress(struct mlx5_ib_srmc *, int, u32 *);

static void reset(u64 cursor, u32 size)
{
    u32 i;
    memset(&cq,0,sizeof(cq)); memset(&ctrl,0,sizeof(ctrl));
    memset(&qp,0,sizeof(qp)); memset(&s,0,sizeof(s));
    memset(entries,0,sizeof(entries));
    core.state=0;
    cq.ibcq.device=&dev; cq.mcq.cqe_sz=size;
    qp.ibqp.qp_num=77; qp.sq.wqe_cnt=1024; qp.sq.tail=(u32)cursor;
    qp.sq.fbc.buf = &sq_entries[0][0];
    memset(sq_entries, 0, sizeof(sq_entries));
    for (i = 0; i < 1024; ++i) {
        u32 op = htonl(0x08), ds = htonl(4), len = htonl((i % 7 + 1) * 512);
        memcpy(sq_entries[i], &op, 4);
        memcpy(sq_entries[i] + 4, &ds, 4);
        memcpy(sq_entries[i] + 48, &len, 4);
    }
    ctrl.route.inflight_limit_bytes = MLX5_SRM_MAX_INFLIGHT_BYTES;
    byte_publishes = 0;
    qp.srmc_owner=&s;
    s.ini_cb.qp=&qp; s.ini_cb.cq=&cq.ibcq; s.ctrl_page=&ctrl;
    s.cq_complete_idx=ctrl.cons_idx=cursor;
    ctrl.completion_error_idx=UINT64_MAX;
    head=ci_stores=publishes=timing_calls=last_ci=0; last_time_post=0;
#if MLX5_SRM_ENABLE_REROUTE
    rr_idle=1; rr_maintenance=rr_failures=rr_slow_calls=0;
    rr_publish_pending=0;
    rr_batch_calls=rr_batch_cqes=0; rr_maintenance_end=0;
    rr_mapped_origin=NULL;
    memset(&rr_origin_ctrl,0,sizeof(rr_origin_ctrl));
    memset(&rr_origin_s,0,sizeof(rr_origin_s));
    rr_origin_s.ctrl_page=&rr_origin_ctrl;
#endif
}
static void add(u64 post, u8 opcode, u32 qpn)
{
    struct mlx5_cqe64 *e=(void *)(entries[head++] + (cq.mcq.cqe_sz==128 ? 64 : 0));
    assert(head <= 2048);
    e->sop_drop_qpn=htonl(qpn); e->wqe_counter=htons((u16)post); e->opcode=opcode;
}
static void finish(int expected, int pending, u32 expected_wqes, u64 cursor)
{
    u32 done=99, before=publishes;
#if !MLX5_SRM_ENABLE_REROUTE && MLX5_SRM_MAX_INFLIGHT_BYTES
    u64 before_bytes = ctrl.route.completed_bytes, delta = 0;
    u32 byte_before = byte_publishes, n;
    for (n = 0; n < expected_wqes; n++)
        delta += mlx5_srm_payload_bytes(sq_entries[(s.cq_complete_idx+n)&1023]);
#endif
    int ret=mlx5_ib_poll_srm_private_progress(&s,pending,&done);
    assert(ret==expected && done==expected_wqes);
    assert(publishes-before == (expected>0 ? 1U : 0U));
    assert(s.cq_complete_idx==cursor && ctrl.cons_idx==cursor && qp.sq.tail==(u32)cursor);
#if !MLX5_SRM_ENABLE_REROUTE && MLX5_SRM_MAX_INFLIGHT_BYTES
    assert(ctrl.route.completed_bytes - before_bytes == delta);
    assert(byte_publishes - byte_before == (expected > 0 ? 1U : 0U));
#endif
}
int main(void)
{
    struct mlx5_ib_sched_worker worker;
    struct mlx5_ib_sched sched={&worker};
    u32 size;
#if !MLX5_SRM_ENABLE_REROUTE && MLX5_SRM_MAX_INFLIGHT_BYTES
    /* Same completion helper used by shared CQ, including unsignaled spans. */
    reset(65534, 64);
    {
        u64 sum = 0; u32 n, done = 0;
        for (n = 0; n < 3; ++n)
            sum += mlx5_srm_payload_bytes(sq_entries[(65534+n)&1023]);
        s.byte_completed = ctrl.route.completed_bytes = UINT64_MAX - 99;
        ctrl.route.posted_bytes = ctrl.route.completed_bytes + sum;
        assert(rr_db_byte_available(&s) == MLX5_SRM_MAX_INFLIGHT_BYTES - sum);
        assert(rr_db_prefix_bytes(&s, 65534, 3, 0, sum) == 0);
        assert(rr_db_prefix_bytes(&s, 65534, 3, 3, sum) == sum);
        assert(rr_db_prefix_bytes(&s, 65534, 3, 1, sum) ==
               mlx5_srm_payload_bytes(sq_entries[1022]));
        assert(rr_db_prefix_bytes(&s, 65534, 3, 2, sum) ==
               sum - mlx5_srm_payload_bytes(sq_entries[0]));
        mlx5_ib_srmc_complete_post(&qp, &qp.sq, 0, &done);
        assert(done == 3 && ctrl.cons_idx == 65534);
        assert(ctrl.route.posted_bytes == ctrl.route.completed_bytes);
        assert(rr_db_byte_available(&s) == MLX5_SRM_MAX_INFLIGHT_BYTES);
    }
#endif
    reset(100,64); ctrl.db_tail=110;
    assert(mlx5_srm_refresh_poll_budget(&sched,&s)==10);
    ctrl.db_tail=100;
    assert(mlx5_srm_refresh_poll_budget(&sched,&s)==0);
    ctrl.db_tail=99;
    assert(mlx5_srm_refresh_poll_budget(&sched,&s)==0);
    reset(UINT64_MAX-1,64); ctrl.db_tail=2;
    assert(mlx5_srm_refresh_poll_budget(&sched,&s)==4);
    s.ctrl_page=NULL;
    assert(mlx5_srm_refresh_poll_budget(&sched,&s)==0);
    sched.worker=NULL;
    assert(mlx5_srm_refresh_poll_budget(&sched,&s)==0);
    for (size=64; size<=128; size*=2) {
        const u64 starts[]={0,65534,UINT32_MAX-1ULL,
#if MLX5_SRM_ENABLE_REROUTE
                            (1ULL<<63)-2};
#else
                            UINT64_MAX-1ULL};
#endif
        unsigned int k;
        reset(0,size); finish(0,10,0,0); assert(!ci_stores);
        reset(7,size); add(7,MLX5_CQE_REQ,77);
        finish(0,0,0,7); finish(1,32,1,8); finish(0,31,0,8);
        for (k=0; k<sizeof(starts)/sizeof(starts[0]); ++k) {
            u64 st=starts[k]; int left=10, n;
            reset(st,size);
            for (n=0;n<10;n++) add(st+n,MLX5_CQE_REQ,77);
            while (left) {
                n=min_t(int,left,MLX5_SRM_PRIVATE_CQ_POLL_BUDGET);
                st+=n;
#if MLX5_SRM_ENABLE_REROUTE
                st=mlx5_srm_rr_seq(st);
#endif
                finish(n,left,n,st); left-=n;
            }
#if MLX5_SRM_ENABLE_WQE_TIMING
            assert(timing_calls==10 && last_time_post==st-1);
#endif
        }
        /* Counter gap: one signaled completion reclaims preceding WRs. */
#if MLX5_SRM_ENABLE_REROUTE
        reset(65534,size); add(65536,MLX5_CQE_REQ,77);
        finish(-ERANGE,3,0,65534); assert(rr_failures);
#else
        reset(65534,size); add(65536,MLX5_CQE_REQ,77); finish(1,3,3,65537);
#endif
        /* More ready CQEs than the DB snapshot must remain unconsumed. */
        reset(0,size); add(0,0,77); add(1,0,77);
        finish(1,1,1,1); assert(cq.mcq.cons_index==1); finish(1,1,1,2);
        /* Short poll, first error, then flush: do not overwrite first error. */
        reset(0,size); add(0,0,77); finish(1,10,1,1);
        add(1,MLX5_CQE_REQ_ERR,77); add(2,MLX5_CQE_REQ_ERR,77);
        finish(1,9,1,2);
        assert(ctrl.completion_error_idx==2 && ctrl.completion_error_status==13);
#if !MLX5_SRM_ENABLE_REROUTE
        finish(1,8,1,3); assert(ctrl.completion_error_idx==2);
#endif
        /* Reject unrelated QP, invalid opcode, stale/out-of-window counter. */
        reset(8,size); add(8,0,88); finish(-EINVAL,1,0,8); assert(!ci_stores);
        reset(8,size); add(8,11,77); finish(-EINVAL,1,0,8);
        reset(8,size); add(7,0,77); finish(-ERANGE,1,0,8);
        reset(8,size); add(10,0,77); finish(-ERANGE,2,0,8);
        reset(8,size); add(8,0,77); add(8,0,77);
        finish(1,2,1,9); finish(-ERANGE,1,0,9);
#if MLX5_SRM_ENABLE_REROUTE
        /* A migrated CQE maps to the original control slot. The physical
         * CQ has advanced, but must not publish that cursor to its users. */
        reset(10,size); rr_idle=0; rr_mapped_origin=&rr_origin_s;
        add(10,MLX5_CQE_REQ,77);
        {
            u32 done=0;
            assert(mlx5_ib_poll_srm_private_progress(&s,1,&done)==1);
            assert(done==1 && rr_slow_calls==1);
            assert(s.cq_complete_idx==11 && ctrl.cons_idx==10);
            assert(rr_origin_ctrl.cons_idx==1 && publishes==1);
        }
        /* The retirement NOP spans several physical BBs but only one
         * issued credit. Its CQE does not report a user completion. */
        reset(20,size); rr_idle=0; rr_maintenance=1;
        rr_maintenance_end=25; ctrl.db_tail=25;
        add(24,MLX5_CQE_REQ,77);
        {
            u32 done=0;
            assert(mlx5_ib_poll_srm_private_progress(&s,5,&done)==1);
            assert(done==1 && s.cq_complete_idx==25 && qp.sq.tail==25);
            assert(ctrl.cons_idx==20 && publishes==0);
            assert(!rr_maintenance && rr_slow_calls==1);
        }
#endif
        /* Resize CQE is administrative, not an SQ completion/credit. */
        reset(0,size); cq.resize_buf=calloc(1,sizeof(*cq.resize_buf));
        add(0,MLX5_CQE_RESIZE_CQ,77); add(0,0,77);
        finish(1,1,1,1); assert(!cq.resize_buf && cq.mcq.cons_index==2);
        reset(0,size); core.state=MLX5_DEVICE_STATE_INTERNAL_ERROR;
        finish(-EIO,1,0,0);
    }
    printf("PASS private CQ production poll: budget=%d timing=%d\n",
           MLX5_SRM_PRIVATE_CQ_POLL_BUDGET, MLX5_SRM_ENABLE_WQE_TIMING);
    return 0;
}
