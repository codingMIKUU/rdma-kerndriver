/* Mock hardware around production private-CQ polling and DB discovery.
 * Kept separate from native dispatch tests: these checks assert one shared
 * watermark publication per nonempty poll, never per CQE. */
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <limits.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
static inline s64 mlx5_srm_seq_delta(u64 a, u64 b)
{
    return (s64)(a - b);
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
};
struct mlx5_ib_srmc;
struct mlx5_ib_wq { u32 tail, wqe_cnt; };
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
static u32 head, ci_stores, publishes, last_ci;

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


static inline u64 mlx5_ib_srmc_complete_post(struct mlx5_ib_qp *,
                                               struct mlx5_ib_wq *, u16, u32 *);
int mlx5_ib_poll_srm_private_progress(struct mlx5_ib_srmc *, int, u32 *);

static void reset(u64 cursor, u32 size)
{
    memset(&cq,0,sizeof(cq)); memset(&ctrl,0,sizeof(ctrl));
    memset(&qp,0,sizeof(qp)); memset(&s,0,sizeof(s));
    memset(entries,0,sizeof(entries));
    core.state=0;
    cq.ibcq.device=&dev; cq.mcq.cqe_sz=size;
    qp.ibqp.qp_num=77; qp.sq.wqe_cnt=1024; qp.sq.tail=(u32)cursor;
    qp.srmc_owner=&s;
    s.ini_cb.qp=&qp; s.ini_cb.cq=&cq.ibcq; s.ctrl_page=&ctrl;
    s.cq_complete_idx=ctrl.cons_idx=cursor;
    ctrl.completion_error_idx=UINT64_MAX;
    head=ci_stores=publishes=last_ci=0;
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
    int ret=mlx5_ib_poll_srm_private_progress(&s,pending,&done);
    assert(ret==expected && done==expected_wqes);
    assert(publishes-before == (expected>0 ? 1U : 0U));
    assert(s.cq_complete_idx==cursor && ctrl.cons_idx==cursor && qp.sq.tail==(u32)cursor);
}
int main(void)
{
    struct mlx5_ib_sched_worker worker;
    struct mlx5_ib_sched sched={&worker};
    u32 size;
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
    sched.worker=&worker;
    for (size=64; size<=128; size*=2) {
        const u64 starts[]={0,65534,UINT32_MAX-1ULL,
                            UINT64_MAX-1ULL};
        unsigned int k;
        reset(0,size); finish(0,10,0,0); assert(!ci_stores);
        reset(7,size); add(7,MLX5_CQE_REQ,77);
        finish(0,0,0,7); finish(1,32,1,8); finish(0,31,0,8);
        for (k=0; k<sizeof(starts)/sizeof(starts[0]); ++k) {
            u64 st=starts[k]; int left=10, n;
            u32 completed;

            /* Native dispatch shares this counter decoder. It must advance
             * the private hardware cursor without freeing SQ slots before
             * the scheduler has copied the CQE to the user CQ. */
            reset(st,size);
            assert(mlx5_ib_srmc_complete_post(&qp, &qp.sq,
                       (u16)(st+2), &completed) == st+2);
            assert(completed==3 && s.cq_complete_idx==st+3);
            assert(ctrl.cons_idx==st && !publishes);
            ctrl.db_tail=st+5;
            assert(mlx5_srm_refresh_poll_budget(&sched,&s)==2);
            reset(st,size);
            for (n=0;n<10;n++) add(st+n,MLX5_CQE_REQ,77);
            while (left) {
                n=min_t(int,left,MLX5_SRM_PRIVATE_CQ_POLL_BUDGET);
                st+=n;
                finish(n,left,n,st); left-=n;
            }
        }
        /* Counter gap: one signaled completion reclaims preceding WRs. */
        reset(65534,size); add(65536,MLX5_CQE_REQ,77); finish(1,3,3,65537);
        /* More ready CQEs than the DB snapshot must remain unconsumed. */
        reset(0,size); add(0,0,77); add(1,0,77);
        finish(1,1,1,1); assert(cq.mcq.cons_index==1); finish(1,1,1,2);
        /* Short poll, first error, then flush: do not overwrite first error. */
        reset(0,size); add(0,0,77); finish(1,10,1,1);
        add(1,MLX5_CQE_REQ_ERR,77); add(2,MLX5_CQE_REQ_ERR,77);
        finish(1,9,1,2);
        assert(ctrl.completion_error_idx==2 && ctrl.completion_error_status==13);
        finish(1,8,1,3); assert(ctrl.completion_error_idx==2);
        /* Reject unrelated QP, invalid opcode, stale/out-of-window counter. */
        reset(8,size); add(8,0,88); finish(-EINVAL,1,0,8); assert(!ci_stores);
        reset(8,size); add(8,11,77); finish(-EINVAL,1,0,8);
        reset(8,size); add(7,0,77); finish(-ERANGE,1,0,8);
        reset(8,size); add(10,0,77); finish(-ERANGE,2,0,8);
        reset(8,size); add(8,0,77); add(8,0,77);
        finish(1,2,1,9); finish(-ERANGE,1,0,9);
        /* Resize CQE is administrative, not an SQ completion/credit. */
        reset(0,size); cq.resize_buf=calloc(1,sizeof(*cq.resize_buf));
        add(0,MLX5_CQE_RESIZE_CQ,77); add(0,0,77);
        finish(1,1,1,1); assert(!cq.resize_buf && cq.mcq.cons_index==2);
        reset(0,size); core.state=MLX5_DEVICE_STATE_INTERNAL_ERROR;
        finish(-EIO,1,0,0);
    }
    printf("PASS private CQ production poll: budget=%d\n",
           MLX5_SRM_PRIVATE_CQ_POLL_BUDGET);
    return 0;
}
