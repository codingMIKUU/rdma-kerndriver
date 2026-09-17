/* Offline tests compile the production state machine with a fake CQ/SQ.
 * This is not an RDMA transport test and does not touch a loaded module. */
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <endian.h>
#include "../include/uapi/rdma/mlx5-srm-reroute.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint64_t atomic64_t;
#define GFP_KERNEL 0
#define PAGE_SIZE 4096
#define NUM_SRMC 4
#define MLX5_SRM_PUBLISH_SEQ_MASK ((1ULL << 48) - 1)
#define MLX5_OPCODE_RDMA_READ 0x10
#define MLX5_OPCODE_RDMA_WRITE 8
#define MLX5_OPCODE_NOP 0
#define MLX5_WQE_CTRL_CQ_UPDATE 8
#define U32_MAX UINT32_MAX
#define module_param(a,b,c)
#define pr_info(...) ((void)0)
#define pr_err(...) ((void)0)
#define READ_ONCE(a) (a)
#define WRITE_ONCE(a,b) ((a)=(b))
#define smp_store_release(a,b) (*(a)=(b))
#define smp_load_acquire(a) (*(a))
#define min_t(t,a,b) ((t)(a)<(t)(b)?(t)(a):(t)(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define time_before(a,b) ((long)((a)-(b))<0)
#define time_after_eq(a,b) (!time_before(a,b))
#define msecs_to_jiffies(a) (a)
#define jiffies_to_msecs(a) ((unsigned)(a))
#define kzalloc(n,f) calloc(1,n)
#define kvcalloc(n,s,f) calloc(n,s)
#define kfree free
#define kvfree free
#define bitmap_free free
#define bitmap_zalloc(n,f) calloc(((n)+63)/64,8)
#define bitmap_zero(p,n) memset(p,0,(((n)+63)/64)*8)
#define __set_bit(n,p) ((p)[(n)/64] |= 1UL<<((n)%64))
#define __clear_bit(n,p) ((p)[(n)/64] &= ~(1UL<<((n)%64)))
#define test_bit(n,p) (((p)[(n)/64]>>((n)%64))&1)
#define cpu_to_be32 htobe32
#define be32_to_cpu be32toh
#define page_address(p) (p)
#define to_mdev(p) (p)
#define MLX5_SRM_DB_OWNER_FREE 0
#define MLX5_SRM_DB_OWNER_KERNEL 2
#define cmpxchg(p,a,b) __extension__ ({ __typeof__(*(p)) old=*(p); if(old==(a)) *(p)=(b); old; })
static u64 atomic64_fetch_or(u64 v, atomic64_t *p) { u64 old=*p; *p|=v; return old; }
#define atomic64_read(p) (*(p))
#define atomic64_cmpxchg cmpxchg
typedef int64_t s64;
static u64 div64_u64(u64 a,u64 b) { return a/b; }
static unsigned long jiffies;
static const int num_kqps=1;

struct mlx5_srm_rr_path;
struct mlx5_srm_rr_group;
struct mlx5_qp_ctrl_pool { void **pages; };
struct dev { struct mlx5_qp_ctrl_pool sq_ctrl_pool; };
struct mlx5_sq_ctrl_page {
    u64 resv_idx,cons_idx,db_tail;
    u32 db_owner,flags;
    u64 completion_error_idx;
    u32 completion_error_status,completion_error_vendor;
    atomic64_t issued_total,completed_total;
    u64 credit_limit;
    u32 bf_offset;
    struct mlx5_srm_route_ctrl route;
};
struct mlx5_wqe_ctrl_seg { u32 opmod_idx_opcode,qpn_ds; u8 rest[3],fm_ce_se,rest2[4]; };
struct mlx5_wqe_data_seg { u32 byte_count,lkey; u64 addr; };
struct fake_sq { u32 wqe_cnt,cur_post,head,tail; void *fbc; };
struct fake_qp { struct fake_sq sq; struct {u32 offset;} bf; struct {u32 qp_num;struct dev *device;} ibqp; };
#define mlx5_ib_qp fake_qp
static void mlx5r_ring_db(struct fake_qp *qp,u32 n,void *last)
{ (void)last;qp->sq.head+=n; }
struct mlx5_ib_srmc {
    struct {struct fake_qp *qp;} ini_cb;
    struct mlx5_sq_ctrl_page *ctrl_page;
    struct mlx5_srm_rr_path *rr;
    int srmc_idx;
    u64 sched_post_idx,cq_complete_idx;
    u32 publish_depth;
    void **publish_pages;
};
struct mlx5_ib_sched {
    struct mlx5_srm_rr_group *rr_groups;
    struct mlx5_ib_srmc *rr_publish_head;
    struct { struct mlx5_sq_ctrl_page *credit_ctrl; } workers[1];
};
static struct {struct mlx5_ib_sched *scheds;} sched_group;
static void *mlx5_frag_buf_get_wqe(void **buf,u32 idx) { return (char *)*buf+64*idx; }
static u64 mlx5_ib_srmc_get_publish_token(struct mlx5_ib_srmc *s,u32 idx)
{ return ((u64 *)s->publish_pages[0])[idx&(s->publish_depth-1)]; }
static u64 mlx5_srm_publish_seq(u64 t) { return t>>16; }
static u16 mlx5_srm_publish_usr_rc(u64 t) { return t; }
#include "../drivers/infiniband/hw/mlx5/reroute.inc"

static struct dev dev;
static struct mlx5_ib_sched sched;
static struct mlx5_ib_srmc paths[4];
static unsigned test_depth=64;
static void init(u64 start)
{
    unsigned p;
    memset(paths,0,sizeof(paths)); memset(&sched,0,sizeof(sched));
    sched_group.scheds=&sched; assert(!rr_sched_init(&sched));
    for(p=0;p<4;p++) {
        struct mlx5_ib_srmc *s=&paths[p];
        s->srmc_idx=p;
        s->ini_cb.qp=calloc(1,sizeof(*s->ini_cb.qp));
        s->ini_cb.qp->sq.wqe_cnt=test_depth;
        s->ini_cb.qp->sq.fbc=calloc(test_depth,64);
        s->ini_cb.qp->ibqp.qp_num=100+p;
        s->ini_cb.qp->ibqp.device=&dev;
        s->ctrl_page=calloc(1,sizeof(*s->ctrl_page));
        s->publish_depth=test_depth;
        s->publish_pages=calloc(1,sizeof(void *));
        s->publish_pages[0]=calloc(1,PAGE_SIZE);
        assert(!rr_path_init(&sched,s));
        s->ctrl_page->cons_idx=s->ctrl_page->db_tail=start;
        s->ctrl_page->resv_idx=start|(p<2?0:MLX5_SRM_REROUTE_FROZEN);
        s->sched_post_idx=s->cq_complete_idx=s->rr->logical_cons=start;
    }
    sched.workers[0].credit_ctrl=paths[0].ctrl_page;
    paths[0].ctrl_page->credit_limit=10000;
}
static void fini(void)
{
    unsigned p;
    for(p=0;p<4;p++) {
        rr_path_free(&paths[p]);
        free(paths[p].ini_cb.qp->sq.fbc);free(paths[p].ini_cb.qp);
        free(paths[p].ctrl_page);
        free(paths[p].publish_pages[0]);free(paths[p].publish_pages);
    }
    rr_sched_free(&sched);
}
static u64 add(u64 s,unsigned n) { return mlx5_srm_rr_seq(s+n); }
static void fill(struct mlx5_ib_srmc *s,u64 idx,u16 user,u32 bytes,bool ready)
{
    struct mlx5_wqe_ctrl_seg *w=rr_wqe(s,idx);
    struct mlx5_wqe_data_seg *d=(void *)w+48;
    w->opmod_idx_opcode=cpu_to_be32(((u16)idx<<8)|MLX5_OPCODE_RDMA_WRITE);
    w->qpn_ds=cpu_to_be32((s->ini_cb.qp->ibqp.qp_num<<8)|4);
    d->byte_count=cpu_to_be32(bytes); d->lkey=cpu_to_be32(123);
    d->addr=0xabcdef;
    if(ready) rr_token(s,idx,user);
    else ((u64 *)s->publish_pages[0])[idx & (s->publish_depth-1)] =
        (((idx + 1 - s->publish_depth) & MLX5_SRM_PUBLISH_SEQ_MASK) << 16) | user;
}
static void complete(struct mlx5_ib_srmc *s,u64 idx,u32 status)
{
    struct mlx5_ib_srmc *origin=s;
    u64 post=idx;
    s->ctrl_page->db_tail=add(idx,1);
    s->cq_complete_idx=add(idx,1);
    mlx5_srm_rr_complete(&origin,&post,status,17,12345);
    mlx5_srm_rr_flush_native(&sched);
}
static void step_to(enum rr_phase phase)
{
    unsigned n=0;
    while(sched.rr_groups[0].phase!=phase) {
        struct mlx5_srm_rr_group *g=&sched.rr_groups[0];
        rr_step(g); jiffies++;
        if(g->phase==RR_NOP_WAIT && g->src->rr->maintenance) {
            u64 post;
            assert(g->src->ctrl_page->route.state==MLX5_SRM_ROUTE_RETIRED);
            assert(!mlx5_srm_rr_maintenance_post(g->src,(u16)g->freeze_end,&post));
            assert(mlx5_srm_rr_maintenance_post(g->src,(u16)(g->freeze_end-1),&post));
            assert(post==mlx5_srm_rr_seq(g->freeze_end-1));
            complete(g->src,post,0);
            sched.workers[0].credit_ctrl->completed_total++;
        }
        assert(++n<200);
    }
}
static void run(u32 total,u64 start)
{
    struct mlx5_srm_rr_group *g;
    struct mlx5_ib_srmc *s,*d;
    bool copying=total>=10240;
    unsigned i;
    init(start);g=&sched.rr_groups[0];s=&paths[0];d=&paths[2];
    /* user 7 has old hardware work; user 8 can migrate without that gate. */
    fill(s,start,7,100,true);
    fill(s,add(start,1),7,total/2,false);
    fill(s,add(start,2),8,total-total/2,true);
    s->ctrl_page->resv_idx=add(start,3); s->ctrl_page->db_tail=add(start,1);
    assert(rr_begin(g,0));
    for(i=0;i<50;i++) rr_step(g);
    assert(g->phase==RR_READY && g->cursor==add(start,1));
    assert(s->ctrl_page->resv_idx&MLX5_SRM_REROUTE_FROZEN);
    assert(!mlx5_srm_rr_can_db(s,add(start,1)));
    rr_token(s,add(start,1),7);
    step_to(copying?RR_COPY0:RR_OLD_DRAIN);
    assert(g->copy==copying && g->bytes==total);
    /* Producers may fill after the prefix but neither can ring here. */
    fill(d,g->dst_end,9,123,true);d->ctrl_page->resv_idx=add(g->dst_end,1);
    assert(!d->ctrl_page->route.user_db);
    assert(!mlx5_srm_rr_can_db(d,g->dst_end));
    if(copying) {
        step_to(RR_COPY_DRAIN);
        assert(mlx5_srm_publish_usr_rc(mlx5_ib_srmc_get_publish_token(d,g->dst_begin))==8);
        assert(mlx5_srm_rr_can_db(d,g->dst_begin));
        assert(!mlx5_srm_rr_can_db(d,add(g->dst_begin,1)));
        complete(d,g->dst_begin,0);
        assert(s->ctrl_page->cons_idx==start); /* no false source watermark */
#if MLX5_SRM_ENABLE_CQE_SIMPLIFY
        assert(rr_record(s,8)->sequence==add(start,3));
        assert(rr_record(s,8)->origin_slot==0);
#endif
        complete(s,start,0);
        rr_step(g);
        assert(d->ctrl_page->route.user_db);
        assert(mlx5_srm_rr_can_db(d,add(g->dst_begin,1)));
        complete(d,add(g->dst_begin,1),0);
        assert(s->ctrl_page->cons_idx==add(start,3));
#if MLX5_SRM_ENABLE_CQE_SIMPLIFY
        assert(rr_record(s,7)->status==0);
        assert(rr_record(s,7)->sequence==add(start,2));
#endif
    } else {
        assert(mlx5_srm_rr_can_db(s,add(start,1)));
        assert(rr_schedule(&sched,0)==0); /* source still scheduled */
        complete(s,start,0);complete(s,add(start,1),0);
        rr_step(g);assert(!d->ctrl_page->route.user_db);
        complete(s,add(start,2),0);rr_step(g);
        assert(d->ctrl_page->route.user_db);
    }
    complete(d,g->dst_end,0);
    step_to(RR_IDLE);
    assert(paths[0].ctrl_page->route.state==MLX5_SRM_ROUTE_SPARE);
    assert(paths[0].ctrl_page->resv_idx&MLX5_SRM_REROUTE_FROZEN);
    assert(paths[0].cq_complete_idx==add(start,3));
    /* Reuse retired paths repeatedly, including zero pending intervals. */
    for(i=0;i<20;i++) {
        assert(rr_begin(g,0));
        rr_step(g); assert(!g->copy && !g->bytes);
        step_to(RR_IDLE);
    }
    fini();
}

static void run_budget_and_errors(void)
{
    struct mlx5_srm_rr_group *g;
    unsigned n;
    test_depth=256;init(0);g=&sched.rr_groups[0];
    paths[0].ctrl_page->resv_idx=160;
    for(n=0;n<160;n++) fill(&paths[0],n,n,1024,true);
    paths[0].ctrl_page->db_owner=1;
    assert(!rr_begin(g,0)); /* no busy-wait or state change */
    paths[0].ctrl_page->db_owner=0;
    assert(rr_begin(g,0));rr_step(g);
    assert(g->phase==RR_READY && g->cursor==64);
    rr_step(g);assert(g->cursor==128);
    step_to(RR_COPY_DRAIN);
    /* Copy/prewrite did not consume any doorbell credit. */
    assert(!paths[0].ctrl_page->issued_total);
    complete(&paths[2],0,13);
    assert(g->phase==RR_FAILED);
    assert(!paths[2].ctrl_page->route.user_db);
    assert(!mlx5_srm_rr_can_db(&paths[2],1));
    assert(g->outstanding==159); /* maps retained for hardware flush CQEs */
#if MLX5_SRM_ENABLE_CQE_SIMPLIFY
    assert(rr_record(&paths[0],0)->status==13);
#endif
    for(n=1;n<160;n++) complete(&paths[2],n,5);
    assert(!g->outstanding);
    rr_step(g);assert(g->phase==RR_FAILED); /* never recycle an ERR QP */
    fini();test_depth=64;
}

static void run_detector(void)
{
    struct mlx5_srm_rr_group *g;
    unsigned i;
    init(0);g=&sched.rr_groups[0];srm_reroute_enable=true;
    jiffies=10000;
    for(i=0;i<2;i++) {
        paths[0].ctrl_page->route.posted_bytes+=1000;
        paths[1].ctrl_page->route.posted_bytes+=1000;
        paths[1].ctrl_page->route.completed_bytes+=1000;
        rr_detect(g);jiffies+=10;
    }
    assert(g->bad[0]==2);
    rr_detect(g);jiffies+=10;assert(!g->bad[0]); /* idle breaks streak */
    for(i=0;i<3;i++) {
        paths[0].ctrl_page->route.posted_bytes+=1000;
        paths[1].ctrl_page->route.posted_bytes+=1000;
        paths[1].ctrl_page->route.completed_bytes+=1000;
        rr_detect(g);jiffies+=10;
        assert(g->phase==(i==2?RR_READY:RR_IDLE));
    }
    srm_reroute_enable=false;
    step_to(RR_IDLE); /* disabling detection does not abandon transaction */
    fini();
}
int main(void)
{
    unsigned i,p;
    dev.sq_ctrl_pool.pages=calloc(513,sizeof(void *));
    for(p=0;p<513;p++) dev.sq_ctrl_pool.pages[p]=calloc(1,PAGE_SIZE);
    assert(sizeof(struct mlx5_srm_route_ctrl)==128);
    assert(sizeof(struct mlx5_srm_migration_completion)==32);
    for(i=0;i<20;i++) {
        u64 starts[]={0,65534,(1ULL<<48)-2,MLX5_SRM_REROUTE_MASK-1};
        unsigned s;
        for(s=0;s<4;s++) {
            run(10239,starts[s]);run(10240,starts[s]);run(10241,starts[s]);
        }
    }
    assert(mlx5_srm_rr_delta(1,MLX5_SRM_REROUTE_MASK)==2);
    run_budget_and_errors();run_detector();
    for(p=0;p<513;p++) free(dev.sq_ctrl_pool.pages[p]);
    free(dev.sq_ctrl_pool.pages);
    printf("PASS: 240 classified + 4800 empty migrations; budgets, owner contention, error mapping, detector, NOP retirement, 16/48/63-bit wrap; simplify=%d\n",
           MLX5_SRM_ENABLE_CQE_SIMPLIFY);
    return 0;
}
