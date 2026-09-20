/* Offline tests compile the production state machine with a fake CQ/SQ.
 * This is not an RDMA transport test and does not touch a loaded module. */
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <endian.h>
#include "../include/uapi/rdma/mlx5-srm-reroute.h"

typedef unsigned long long u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef u64 atomic64_t;
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
#define MODULE_PARM_DESC(a,b)
static char timing_log[2048];
static u64 fake_cycles, fake_log_cycles;
static void (*log_hook)(const char *line);
static void record_info(const char *fmt, ...)
{
    char line[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    fake_cycles += fake_log_cycles;
    if (!strncmp(line, "SRM_REROUTE_TIMING ", 18))
        memcpy(timing_log, line, strlen(line) + 1);
    if (log_hook) log_hook(line);
}
#define pr_info(...) record_info(__VA_ARGS__)
#define pr_err(...) ((void)0)
#define READ_ONCE(a) (a)
#define WRITE_ONCE(a,b) ((a)=(b))
#define likely(a) (a)
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
static u64 bitmap_reads, token_reads, descriptor_reads;
#define test_bit(n,p) (bitmap_reads++, (((p)[(n)/64]>>((n)%64))&1))
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
#define NSEC_PER_USEC 1000ULL
static u64 ktime_get_ns(void) { return (u64)jiffies * 1000000; }
static u64 rdtsc_ordered(void) { return fake_cycles += 100; }
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
static void *mlx5_frag_buf_get_wqe(void **buf,u32 idx)
{ descriptor_reads++; return (char *)*buf+64*idx; }
static u64 mlx5_ib_srmc_get_publish_token(struct mlx5_ib_srmc *s,u32 idx)
{ token_reads++; return ((u64 *)s->publish_pages[0])[idx&(s->publish_depth-1)]; }
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
    timing_log[0] = '\0';
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
        assert(s->ctrl_page->route.completed_bytes == 0);
        assert(s->ctrl_page->route.inflight_limit_bytes ==
               MLX5_SRM_MAX_INFLIGHT_BYTES);
        /* The physical cursor slot is still reserved. */
        s->ctrl_page->route.physical_cons=0x87654321;
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
        assert(paths[p].ctrl_page->route.inflight_limit_bytes ==
               MLX5_SRM_MAX_INFLIGHT_BYTES);
        assert(paths[p].ctrl_page->route.physical_cons==0x87654321);
        rr_path_free(&paths[p]);
        free(paths[p].ini_cb.qp->sq.fbc);free(paths[p].ini_cb.qp);
        free(paths[p].ctrl_page);
        free(paths[p].publish_pages[0]);free(paths[p].publish_pages);
    }
    rr_sched_free(&sched);
}
static u64 add(u64 s,unsigned n) { return mlx5_srm_rr_seq(s+n); }
static void check_timing(u64 expected, const char *endpoint)
{
    u64 total, count, average, before, after;
    unsigned valid;
    const char *field;

    assert((field = strstr(timing_log, " total_cycles=")));
    assert(sscanf(field, " total_cycles=%llu", &total) == 1 && total > 0);
    assert((field = strstr(timing_log, " switch_to_user_prewrite_cycles=")));
    assert(sscanf(field, " switch_to_user_prewrite_cycles=%llu", &before) == 1);
    assert((field = strstr(timing_log, " user_prewrite_to_kernel_db_cycles=")));
    assert(sscanf(field, " user_prewrite_to_kernel_db_cycles=%llu", &after) == 1);
    assert(total == before + after);
    assert(strstr(timing_log, "timing_scope=reroute_work_tsc"));
    assert(strstr(timing_log, "blocked_undb_scope=source_frozen_undb_wqes"));
    assert(strstr(timing_log, "blocked_undb_avg_scope=reroute_work_per_source_undb_wqe"));
    assert((field = strstr(timing_log, " blocked_undb_wqes=")));
    assert(sscanf(field, " blocked_undb_wqes=%llu", &count) == 1);
    assert(count == expected);
    assert((field = strstr(timing_log, " blocked_undb_avg_cycles=")));
    assert(sscanf(field, " blocked_undb_avg_cycles=%llu", &average) == 1);
    assert(average == (count ? total / count : 0));
    assert((field = strstr(timing_log, " blocked_undb_avg_valid=")));
    assert(sscanf(field, " blocked_undb_avg_valid=%u", &valid) == 1);
    assert(valid == !!count);
    assert(strstr(timing_log, endpoint));
    assert(!strstr(timing_log, " transition_"));
}
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
static int complete(struct mlx5_ib_srmc *s,u64 idx,u32 status)
{
    struct mlx5_ib_srmc *origin=s;
    struct mlx5_ib_srmc *expected_origin=s;
    u64 post=idx;
    u64 expected_post=idx;
    u64 reads=token_reads;
    int ret;
    if(s->rr->meta) {
        struct rr_meta *m=&s->rr->meta[idx&(s->ini_cb.qp->sq.wqe_cnt-1)];
        if(m->origin) { expected_origin=m->origin;expected_post=m->post; }
    }
    /* Hardware completion cannot rewind an already-posted DB prefix. */
    if (mlx5_srm_rr_delta(add(idx,1), s->ctrl_page->db_tail) > 0)
        s->ctrl_page->db_tail=add(idx,1);
    s->cq_complete_idx=add(idx,1);
    ret=mlx5_srm_rr_complete(&origin,&post,status,17,12345);
    if (MLX5_SRM_MAX_INFLIGHT_BYTES)
        assert(s->ctrl_page->route.completed_bytes == s->rr->completed_bytes);
    assert(origin==expected_origin && post==expected_post);
#if !MLX5_SRM_ENABLE_CQE_SIMPLIFY
    /* Native CQ routing, not rr_complete, owns token validation/lookup. */
    assert(token_reads==reads);
#else
    (void)reads;
#endif
    mlx5_srm_rr_flush_native(&sched);
    return ret;
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
/* Emulate a concurrent producer at route publication, without requiring
 * rr_step() to yield artificially between RESERVE and COPY/OLD_DRAIN. */
static unsigned prewrite_count;
static void prewrite_on_log(const char *line)
{
    struct mlx5_srm_rr_group *g = &sched.rr_groups[0];
    unsigned n;
    if (!strstr(line, "event=prefill-open ")) return;
    assert(g->timing_prewrite_open && !g->timing_running);
    assert(!g->dst->ctrl_page->route.user_db && !g->copying_done);
    for (n = 0; n < prewrite_count; n++)
        fill(g->dst, add(g->dst_end, n), 9, 123, true);
    g->dst->ctrl_page->resv_idx = add(g->dst_end, prewrite_count);
    assert(!mlx5_srm_rr_can_db(g->dst, g->dst_end));
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
    s->ctrl_page->route.posted_bytes = 100;
    assert(rr_begin(g,0));
    for(i=0;i<50;i++) rr_step(g);
    assert(g->phase==RR_READY && g->cursor==add(start,1));
    assert(s->ctrl_page->resv_idx&MLX5_SRM_REROUTE_FROZEN);
    assert(!mlx5_srm_rr_can_db(s,add(start,1)));
    rr_token(s,add(start,1),7);
    prewrite_count = 1; log_hook = prewrite_on_log;
    rr_step(g);
    log_hook = NULL;
    assert(g->phase == (copying ? RR_COPY_DRAIN : RR_OLD_DRAIN));
    assert(g->copy==copying && g->bytes==total);
    /* Prewrite was allowed before copying, but its DB is still gated. */
    assert(!d->ctrl_page->route.user_db);
    assert(!mlx5_srm_rr_can_db(d,g->dst_end));
    if(copying) {
        step_to(RR_COPY_DRAIN);
        check_timing(2, "timing_end=copy_ready"); /* two copies, exclude prewrite */
        assert(mlx5_srm_publish_usr_rc(mlx5_ib_srmc_get_publish_token(d,g->dst_begin))==8);
        assert(mlx5_srm_rr_can_db(d,g->dst_begin));
        assert(!mlx5_srm_rr_can_db(d,add(g->dst_begin,1)));
        d->ctrl_page->route.posted_bytes += rr_bytes(d, g->dst_begin);
        complete(d,g->dst_begin,0);
        assert(s->ctrl_page->cons_idx==start); /* no false source watermark */
        /* Migrated completion releases physical destination bytes only. */
        assert(s->ctrl_page->route.posted_bytes == 100);
        assert(s->ctrl_page->route.completed_bytes == 0);
        if (MLX5_SRM_MAX_INFLIGHT_BYTES)
            assert(d->ctrl_page->route.completed_bytes ==
                   d->ctrl_page->route.posted_bytes);
#if MLX5_SRM_ENABLE_CQE_SIMPLIFY
        assert(rr_record(s,8)->sequence==add(start,3));
        assert(rr_record(s,8)->origin_slot==0);
#endif
        complete(s,start,0);
        rr_step(g);
        assert(d->ctrl_page->route.user_db);
        assert(mlx5_srm_rr_can_db(d,add(g->dst_begin,1)));
        d->ctrl_page->route.posted_bytes += rr_bytes(d, add(g->dst_begin, 1));
        complete(d,add(g->dst_begin,1),0);
        assert(s->ctrl_page->cons_idx==add(start,3));
#if MLX5_SRM_ENABLE_CQE_SIMPLIFY
        assert(rr_record(s,7)->status==0);
        assert(rr_record(s,7)->sequence==add(start,2));
#endif
    } else {
        assert(mlx5_srm_rr_can_db(s,add(start,1)));
        assert(rr_schedule(&sched,0,ktime_get_ns())==0); /* source still scheduled */
        assert(!g->timing_reported && !g->drain_db_ready);
        /* Posting old work is enough to enable dependency-checked target
         * DBs. User 9 has no old work and need not wait for users 7/8. */
        s->ctrl_page->db_tail = g->freeze_end;
        s->ctrl_page->route.posted_bytes += total;
        rr_step(g);
        assert(g->drain_db_ready && !d->ctrl_page->route.user_db);
        assert(rr_schedule(&sched,0,ktime_get_ns())==2);
        assert(mlx5_srm_rr_can_db(d,g->dst_end));
        check_timing(2, "timing_end=drain_ready"); /* frozen suffix, exclude prewrite */
        complete(s,start,0);complete(s,add(start,1),0);
        rr_step(g);assert(!d->ctrl_page->route.user_db);
        complete(s,add(start,2),0);rr_step(g);
        assert(d->ctrl_page->route.user_db);
        check_timing(2, "timing_end=drain_ready"); /* fixed count, no second report */
    }
    d->ctrl_page->route.posted_bytes += 123;
    complete(d,g->dst_end,0);
    step_to(RR_IDLE);
    assert(s->rr->completed_bytes==100+(copying?0:total));
    assert(d->rr->completed_bytes==123+(copying?total:0));
    /* Retirement NOPs and repeated empty migrations neither charge bytes
     * nor refund the copied payload twice on the source. */
    assert(s->ctrl_page->route.posted_bytes == s->rr->completed_bytes);
    assert(d->ctrl_page->route.posted_bytes == d->rr->completed_bytes);
    assert(paths[0].ctrl_page->route.state==MLX5_SRM_ROUTE_SPARE);
    assert(paths[0].ctrl_page->resv_idx&MLX5_SRM_REROUTE_FROZEN);
    assert(paths[0].cq_complete_idx==add(start,3));
    /* Reuse retired paths repeatedly, including zero pending intervals. */
    for(i=0;i<20;i++) {
        assert(rr_begin(g,0));
        rr_step(g); assert(!g->copy && !g->bytes);
        step_to(RR_IDLE);
        check_timing(0, "timing_end=drain_ready");
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
    step_to(RR_RECYCLE); /* no old in-flight dependency: target opens now */
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
        paths[1].rr->completed_bytes+=1000;
        rr_detect(g,ktime_get_ns());jiffies+=10;
    }
    assert(g->bad[0]==2);
    rr_detect(g,ktime_get_ns());jiffies+=10;assert(!g->bad[0]); /* idle breaks streak */
    for(i=0;i<3;i++) {
        paths[0].ctrl_page->route.posted_bytes+=1000;
        paths[1].ctrl_page->route.posted_bytes+=1000;
        paths[1].rr->completed_bytes+=1000;
        rr_detect(g,ktime_get_ns());jiffies+=10;
        assert(g->phase==(i==2?RR_READY:RR_IDLE));
    }
    srm_reroute_enable=false;
    step_to(RR_IDLE); /* disabling detection does not abandon transaction */
    fini();
}

static void run_normal_fast_path(void)
{
    u64 starts[]={0,65535,(1ULL<<48)-1,MLX5_SRM_REROUTE_MASK};
    unsigned i,enabled;
    for(enabled=0;enabled<2;enabled++) for(i=0;i<4;i++) {
        struct mlx5_ib_srmc *s;
        struct rr_meta *meta;
        unsigned long *done;
        void **tokens;
        u64 start=starts[i],bits,reads;
        init(start);s=&paths[0];srm_reroute_enable=enabled;
        fill(s,start,7,1234,true);
        meta=s->rr->meta;done=s->rr->done;tokens=s->publish_pages;
        s->rr->meta=NULL;s->rr->done=NULL;s->publish_pages=NULL;
        bits=bitmap_reads;reads=token_reads;
        /* A never-migrated successful CQE must not touch any of these. */
        assert(complete(s,start,0)==0);
        assert(bitmap_reads==bits && token_reads==reads);
        assert(s->ctrl_page->cons_idx==add(start,1));
        assert(s->rr->logical_cons==add(start,1));
        assert(s->rr->completed_bytes==1234);
        s->rr->meta=meta;s->rr->done=done;s->publish_pages=tokens;
        fini();
    }
    /* Errors must not take the successful fast path even in an idle group. */
    init(0);fill(&paths[0],0,7,99,true);
    assert(complete(&paths[0],0,13)==0);
    assert(sched.rr_groups[0].phase==RR_FAILED);
    assert(paths[0].ctrl_page->completion_error_status==13);
#if MLX5_SRM_ENABLE_CQE_SIMPLIFY
    assert(rr_record(&paths[0],7)->status==13);
#endif
    fini();
}

#if MLX5_SRM_ENABLE_PRIVATE_CQ && MLX5_SRM_ENABLE_CQE_SIMPLIFY
static void run_private_idle_batch(void)
{
    const u64 starts[] = {0, 65534, MLX5_SRM_REROUTE_MASK - 1};
    unsigned int k, enabled;

    for (enabled = 0; enabled < 2; enabled++) {
        for (k = 0; k < sizeof(starts) / sizeof(starts[0]); k++) {
            struct mlx5_ib_srmc *s;
            struct rr_meta *meta;
            unsigned long *done;
            void **tokens;
            u64 before, reads;

            init(starts[k]);
            s = &paths[0];
            srm_reroute_enable = enabled;
            fill(s, starts[k], 1, 101, true);
            fill(s, add(starts[k], 1), 2, 202, true);
            fill(s, add(starts[k], 2), 3, 303, true);
            meta = s->rr->meta;
            done = s->rr->done;
            tokens = s->publish_pages;
            s->rr->meta = NULL;
            s->rr->done = NULL;
            s->publish_pages = NULL;
            before = s->ctrl_page->cons_idx;
            reads = descriptor_reads;
            assert(mlx5_srm_rr_idle(s));
            mlx5_srm_rr_complete_idle_batch(s, starts[k], 3);
            assert(s->ctrl_page->cons_idx == before);
            assert(s->rr->logical_cons == add(starts[k], 3));
            assert(s->rr->completed_bytes ==
                   ((MLX5_SRM_MAX_INFLIGHT_BYTES || enabled) ? 606 : 0));
            assert(descriptor_reads - reads ==
                   ((MLX5_SRM_MAX_INFLIGHT_BYTES || enabled) ? 3 : 0));
            if (MLX5_SRM_MAX_INFLIGHT_BYTES)
                assert(s->ctrl_page->route.completed_bytes == 606);
            mlx5_srm_rr_flush_native(&sched);
            assert(s->ctrl_page->cons_idx == add(starts[k], 3));
            assert(!s->rr->publish_pending);
            s->rr->meta = meta;
            s->rr->done = done;
            s->publish_pages = tokens;
            fini();
        }
    }
}
#endif

static void run_db_byte_prefix(void)
{
    u64 starts[]={0,60,65530,(1ULL<<48)-6,MLX5_SRM_REROUTE_MASK-5};
    unsigned i,n,grant;
    for(i=0;i<5;i++) {
        u64 start=starts[i],total=0,expected=0;
        init(start);
        for(n=0;n<12;n++) {
            u32 bytes=(n+1)*123;
            fill(&paths[0],add(start,n),n,bytes,true);total+=bytes;
        }
        for(grant=0;grant<=12;grant++) {
            u64 reads=descriptor_reads;
            assert(rr_db_prefix_bytes(&paths[0],start,12,grant,total)==expected);
            assert(descriptor_reads-reads==(grant<12-grant?grant:12-grant));
            if(grant<12) expected+=(grant+1)*123;
        }
        fini();
    }
}
static void run_empty_source_prewrite(void)
{
    const u64 starts[] = {0, 65535, (1ULL << 48) - 1,
                         MLX5_SRM_REROUTE_MASK};
    unsigned i;

    for (i = 0; i < sizeof(starts) / sizeof(starts[0]); i++) {
        struct mlx5_srm_rr_group *g;
        struct mlx5_ib_srmc *d;
        init(starts[i]);
        g = &sched.rr_groups[0];
        d = &paths[2];
        assert(rr_begin(g, 0));
        prewrite_count = 5; log_hook = prewrite_on_log;
        rr_step(g);
        log_hook = NULL;
        assert(g->phase == RR_IDLE); /* no forced yield for empty source */
        assert(g->freeze_end == g->db_stop);
        assert(d->ctrl_page->route.user_db);
        assert(g->copied == 0);
        /* Five target prewrites must not turn an empty source suffix into
         * a nonzero denominator, including across cursor wrap. */
        check_timing(0, "timing_end=drain_ready");
        fini();
    }
}

static u64 run_work_timing(bool copy, u64 unrelated_cycles)
{
    struct mlx5_srm_rr_group *g;
    u64 total, before;
    unsigned n;

    init(0); g = &sched.rr_groups[0]; jiffies = 10000;
    fake_log_cycles = unrelated_cycles;
    fill(&paths[0], 0, 7, 100, true);
    fill(&paths[0], 1, 7, copy ? 10240 : 512, false);
    paths[0].ctrl_page->resv_idx = 2;
    paths[0].ctrl_page->db_tail = 1;
    assert(rr_begin(g, 0));
    assert(!g->timing_running);
    for (n = 0; n < 3; n++) {
        before = g->timing_before_prewrite;
        fake_cycles += unrelated_cycles; /* other groups, CQ/DB, waiting */
        assert(g->timing_before_prewrite == before);
        rr_step(g);
        assert(g->phase == RR_READY && !g->timing_running);
    }
    rr_token(&paths[0], 1, 7);
    /* A busy destination must still yield rather than spin. */
    paths[2].ctrl_page->db_owner = 1;
    rr_step(g);
    assert(g->phase == RR_RESERVE && !g->timing_running);
    fake_cycles += unrelated_cycles;
    paths[2].ctrl_page->db_owner = 0;
    prewrite_count = 1; log_hook = prewrite_on_log;
    rr_step(g); log_hook = NULL;
    assert(g->phase == (copy ? RR_COPY_DRAIN : RR_OLD_DRAIN));
    if (!copy) {
        assert(!g->timing_reported);
        paths[0].ctrl_page->db_tail = g->freeze_end;
        rr_step(g);
        assert(g->drain_db_ready);
    }
    assert(g->timing_reported);
    total = g->timing_before_prewrite + g->timing_after_prewrite;
    for (n = 0; n < 3; n++) {
        fake_cycles += unrelated_cycles;
        rr_step(g);
        assert(!g->timing_running);
        assert(total == g->timing_before_prewrite + g->timing_after_prewrite);
    }
    complete(&paths[0], 0, 0);
    if (!copy) complete(&paths[0], 1, 0);
    fake_cycles += unrelated_cycles;
    rr_step(g);
    assert(paths[2].ctrl_page->route.user_db);
    check_timing(1, copy ? "timing_end=copy_ready" :
                           "timing_end=drain_ready");
    total = g->timing_before_prewrite + g->timing_after_prewrite;
    /* Reporting/cleanup cannot keep adding cycles to the closed window. */
    fake_cycles += unrelated_cycles;
    rr_step(g);
    assert(total == g->timing_before_prewrite + g->timing_after_prewrite);
    assert(!g->timing_running);
    fake_log_cycles = 0;
    fini();
    return total;
}

static void run_phase_budget_boundary(void)
{
    struct mlx5_srm_rr_group *g;
    u64 reads;
    unsigned n;

    test_depth = 256; init(0); g = &sched.rr_groups[0];
    fill(&paths[0], 0, 7, 100, true);
    for (n = 1; n <= 64; n++) fill(&paths[0], n, 8, 1024, true);
    paths[0].ctrl_page->resv_idx = 65;
    paths[0].ctrl_page->db_tail = 1;
    assert(rr_begin(g, 0));
    rr_step(g);
    assert(g->phase == RR_SNAPSHOT && g->cursor == 0);
    /* Zero budget at a phase boundary must not wrap to UINT_MAX. */
    reads = token_reads;
    rr_step(g);
    assert(token_reads - reads == 64); /* snapshot 1 + COPY0 63 */
    assert(g->phase == RR_COPY0 && g->copied == 63 && g->cursor == 64);
    reads = token_reads;
    rr_step(g);
    assert(token_reads - reads == 64); /* COPY0 1 + COPY1 63 */
    assert(g->phase == RR_COPY1 && g->copied == 64 && g->cursor == 64);
    rr_step(g);
    assert(g->phase == RR_COPY_DRAIN && g->copying_done);
    check_timing(64, "timing_end=copy_ready");
    fini(); test_depth = 64;

    /* Detection, empty classification, route publication and recycle can
     * all finish on the same scheduler visit, not on successive sweeps. */
    init(0); g = &sched.rr_groups[0];
    srm_reroute_test_source = 0;
    assert(rr_schedule(&sched, 0, ktime_get_ns()) == 2);
    assert(srm_reroute_test_source == -1 && g->phase == RR_IDLE);
    check_timing(0, "timing_end=drain_ready");
    fini();
}

static void run_drain_dependencies(u64 start)
{
    struct mlx5_srm_rr_group *g;
    struct mlx5_ib_srmc *s, *d;
    u64 cycles;
    unsigned n;

    init(start); g = &sched.rr_groups[0]; s = &paths[0]; d = &paths[2];
    fill(s, start, 7, 100, true);          /* already DB'd, not complete */
    fill(s, add(start, 1), 8, 100, true); /* not DB'd at freeze */
    fill(s, add(start, 2), 7, 100, true); /* later dependency for user 7 */
    s->ctrl_page->db_tail = add(start, 1);
    s->ctrl_page->resv_idx = add(start, 3);
    assert(rr_begin(g, 0)); rr_step(g);
    assert(!g->copy && g->phase == RR_OLD_DRAIN);
    assert(test_bit(7, g->dependent) && test_bit(8, g->dependent));
    assert(g->dep_end[7] == add(start, 3));
    assert(g->dep_end[8] == add(start, 2));
    assert(!test_bit(9, g->dependent));

    fill(d, start, 9, 100, true);          /* independent head */
    fill(d, add(start, 1), 7, 100, true); /* waits for LAST user-7 old WR */
    fill(d, add(start, 2), 9, 100, true); /* independent, behind a gate */
    fill(d, add(start, 3), 8, 100, true);
    fill(d, add(start, 4), 9, 100, false); /* incomplete token */
    d->ctrl_page->resv_idx = add(start, 5);
    assert(!mlx5_srm_rr_can_db(d, start));
    s->ctrl_page->db_tail = add(start, 2); /* partial source DB isn't enough */
    assert(rr_schedule(&sched, 0, ktime_get_ns()) == 0);
    assert(!g->drain_db_ready && !g->timing_reported);
    s->ctrl_page->db_tail = g->freeze_end;
    assert(rr_schedule(&sched, 0, ktime_get_ns()) == 2);
    assert(g->drain_db_ready && !d->ctrl_page->route.user_db);
    assert(s->cq_complete_idx == start);  /* no old completion yet */
    /* Two frozen source WQEs, not the five target prewrites or the one
     * source request already DB'd before freezing. Partial DBs do not
     * shrink the denominator when the timing endpoint is reached. */
    check_timing(2, "timing_end=drain_ready");
    cycles = g->timing_before_prewrite + g->timing_after_prewrite;

    /* The scheduler scans a continuous prefix and stops at the first gate:
     * it cannot skip user 7 just because the next user 9 is independent. */
    for (n = 0; n < 5 && mlx5_srm_rr_can_db(d, add(start, n)); n++) {}
    assert(n == 1);
    d->ctrl_page->db_tail = add(start, 1);
    assert(complete(d, start, 0) == 0); /* early target completion is native */
    assert(d->ctrl_page->cons_idx == add(start, 1));
    assert(s->ctrl_page->cons_idx == start && !g->outstanding);
    complete(s, start, 0);
    assert(!mlx5_srm_rr_can_db(d, add(start, 1))); /* user 7 still has suffix */
    assert(!mlx5_srm_rr_can_db(d, add(start, 3))); /* user 8's old WR */
    complete(s, add(start, 1), 0);
    assert(mlx5_srm_rr_can_db(d, add(start, 3)));  /* user 8 dependency gone */
    assert(!mlx5_srm_rr_can_db(d, add(start, 1)));
    assert(!mlx5_srm_rr_can_db(d, add(start, 4))); /* no token, no DB */
    for (n = 0; n < 50; n++) {
        fake_cycles += 1000000;
        rr_step(g);
        assert(g->timing_before_prewrite + g->timing_after_prewrite == cycles);
    }

    /* An owner conflict when opening unrestricted user DB must not undo
     * the already-published dependency-gated kernel DB permission. */
    d->ctrl_page->db_owner = 1;
    complete(s, add(start, 2), 0); rr_step(g);
    assert(g->phase == RR_OLD_DRAIN && !d->ctrl_page->route.user_db);
    assert(mlx5_srm_rr_can_db(d, add(start, 1)));
    assert(!mlx5_srm_rr_can_db(d, add(start, 4)));
    d->ctrl_page->db_owner = 0; rr_step(g);
    assert(g->phase == RR_IDLE && d->ctrl_page->route.user_db);
    assert(g->timing_before_prewrite + g->timing_after_prewrite == cycles);
    check_timing(2, "timing_end=drain_ready");
    for (n = 1; n < 4; n++) complete(d, add(start, n), 0);
    assert(d->ctrl_page->cons_idx == add(start, 4));
    fini();
}

static void run_drain_snapshot_budget(void)
{
    struct mlx5_srm_rr_group *g;
    unsigned n;
    u64 cycles;

    test_depth = 256; init(0); g = &sched.rr_groups[0];
    /* Empty un-DB'd interval still has 130 old dependencies. Threshold=1
     * cannot turn this into a copy: those WRs are already on the NIC. */
    srm_reroute_copy_threshold_bytes = 1;
    for (n = 0; n < 130; n++) fill(&paths[0], n, n, 4096, true);
    paths[0].ctrl_page->db_tail = paths[0].ctrl_page->resv_idx = 130;
    assert(rr_begin(g, 0)); rr_step(g);
    assert(!g->copy && g->phase == RR_SNAPSHOT && g->cursor == 64);
    assert(paths[0].ctrl_page->route.active[0] == 0);
    complete(&paths[0], 0, 0); /* CQ advances between bounded snapshot slices */
    rr_step(g);
    assert(g->phase == RR_SNAPSHOT && g->cursor == 128);
    rr_step(g);
    assert(g->phase == RR_OLD_DRAIN && g->drain_db_ready);
    assert(!g->copy && g->bytes == 0 && g->freeze_end == g->db_stop);
    assert(g->dep_end[129] == 130);
    check_timing(0, "timing_end=drain_ready");
    cycles = g->timing_before_prewrite + g->timing_after_prewrite;
    fill(&paths[2], 0, 129, 100, true);
    paths[2].ctrl_page->resv_idx = 1;
    assert(!mlx5_srm_rr_can_db(&paths[2], 0));
    complete(&paths[0], 1, 13);
    assert(g->phase == RR_FAILED);
    assert(!mlx5_srm_rr_can_db(&paths[2], 0));
    rr_step(g);
    assert(g->timing_before_prewrite + g->timing_after_prewrite == cycles);
    fini(); test_depth = 64;

    /* The same threshold with even ONE un-DB'd byte does select copy. */
    init(0); g = &sched.rr_groups[0];
    fill(&paths[0], 0, 7, 1, true);
    paths[0].ctrl_page->resv_idx = 1;
    assert(rr_begin(g, 0)); rr_step(g);
    assert(g->copy && g->copied == 1);
    check_timing(1, "timing_end=copy_ready");
    fini(); srm_reroute_copy_threshold_bytes = 10240;
}

/* DB accounting model calls the exact shared fit helper/kernel credit
 * snapshot, including the actual partial-worker-grant correction. The
 * production scheduler itself is separately compiled with both CQ modes. */
static unsigned byte_window_db(struct mlx5_ib_srmc *s, unsigned max_wqes,
                               unsigned worker_grant)
{
    u64 first = s->ctrl_page->db_tail, available, bytes = 0;
    unsigned n = 0, grant;
    assert(rr_lock(s));
    available = rr_db_byte_available(s);
    while (n < max_wqes) {
        u32 payload = rr_bytes(s, add(first, n));
        if (!mlx5_srm_rr_byte_can_post(available, bytes, payload,
                                      s->ctrl_page->route.inflight_limit_bytes, n))
            break;
        bytes += payload;
        n++;
    }
    grant = n < worker_grant ? n : worker_grant;
    bytes = rr_db_prefix_bytes(s, first, n, grant, bytes);
    s->ctrl_page->route.posted_bytes += bytes;
    s->ctrl_page->db_tail = add(first, grant);
    rr_unlock(s);
    return grant;
}

static void run_byte_window(void)
{
    const u32 cap = 32768;
    const u64 starts[] = {0, 65534, (1ULL << 48) - 2,
                         MLX5_SRM_REROUTE_MASK - 1};
    unsigned k, i;
    /* Pure accounting helpers used by BOTH user DB and scheduler DB. */
    assert(mlx5_srm_rr_byte_available(0, 0, cap) == cap);
    assert(mlx5_srm_rr_byte_available(cap, 0, cap) == 0);
    assert(mlx5_srm_rr_byte_available(cap, 8192, cap) == 8192);
    assert(mlx5_srm_rr_byte_available(cap + 1, 0, cap) == 0);
    assert(mlx5_srm_rr_byte_available(7, 8, cap) == 0); /* fail closed */
    assert(mlx5_srm_rr_byte_available(10, UINT64_MAX - 9, cap) == cap - 20);
    assert(mlx5_srm_rr_byte_available(UINT64_MAX, 0, 0) == UINT64_MAX);
    assert(mlx5_srm_rr_byte_fits(cap, cap - 1, 1));
    assert(!mlx5_srm_rr_byte_fits(cap, cap - 1, 2));
    assert(!mlx5_srm_rr_byte_fits(cap, UINT64_MAX, 1));
    assert(mlx5_srm_rr_byte_can_post(cap, 0, cap, cap, 0));
    assert(mlx5_srm_rr_byte_can_post(cap, 0, cap + 1, cap, 0));
    assert(!mlx5_srm_rr_byte_can_post(cap - 1, 0, cap + 1, cap, 0));
    assert(!mlx5_srm_rr_byte_can_post(0, 0, cap + 1, cap, 0));
    assert(!mlx5_srm_rr_byte_can_post(cap, 0, cap + 1, cap, 1));
    assert(!mlx5_srm_rr_byte_can_post(cap, 1, cap + 1, cap, 0));
    assert(!mlx5_srm_rr_byte_can_post(cap, cap + 1, 0, cap, 1));
    assert(!mlx5_srm_rr_byte_can_post(4096, 0, 8192, cap, 0));
    assert(mlx5_srm_rr_byte_can_post(UINT64_MAX, cap + 1, UINT32_MAX, 0, 1));
    assert(mlx5_srm_rr_byte_can_post(UINT64_MAX, cap + 1, UINT32_MAX, cap, 1));
    if (MLX5_SRM_MAX_INFLIGHT_BYTES != cap) return;

    for (k = 0; k < sizeof(starts) / sizeof(starts[0]); k++) {
        struct mlx5_ib_srmc *s, *other;
        u64 start = starts[k];
        init(start); s = &paths[0]; other = &paths[1];
        srm_reroute_enable = false; /* cap must work with detection disabled */
        for (i = 0; i < 8; i++) fill(s, add(start, i), i, 8192, true);
        fill(other, start, 9, cap, true);
        assert(byte_window_db(s, 8, 8) == 4);
        assert(rr_db_byte_available(s) == 0);
        assert(byte_window_db(s, 4, 4) == 0); /* next owner shares SAME cap */
        assert(byte_window_db(other, 1, 1) == 1); /* independent QP */
        complete(s, start, 0); /* free exactly one physical payload */
        assert(rr_db_byte_available(s) == 8192);
        assert(byte_window_db(s, 4, 4) == 1);
        for (i = 1; i < 5; i++) complete(s, add(start, i), 0);
        assert(rr_db_byte_available(s) == cap);
        /* Scanned three but only one worker credit: no phantom byte charge. */
        assert(byte_window_db(s, 3, 1) == 1);
        assert(rr_db_byte_available(s) == cap - 8192);
        complete(s, add(start, 5), 0);
        assert(rr_db_byte_available(s) == cap);
        fini();
    }
    /* Counter reuse/wrap and heterogeneous lengths. */
    init(0);
    paths[0].ctrl_page->route.posted_bytes = UINT64_MAX - 999;
    paths[0].ctrl_page->route.completed_bytes = UINT64_MAX - 999;
    paths[0].rr->completed_bytes = UINT64_MAX - 999;
    fill(&paths[0], 0, 0, 1000, true);
    fill(&paths[0], 1, 1, cap - 1000, true);
    fill(&paths[0], 2, 2, 1, true);
    assert(byte_window_db(&paths[0], 3, 3) == 2);
    assert(!rr_db_byte_available(&paths[0]));
    complete(&paths[0], 0, 0);
    assert(rr_db_byte_available(&paths[0]) == 1000);
    assert(byte_window_db(&paths[0], 1, 1) == 1);
    complete(&paths[0], 1, 0); complete(&paths[0], 2, 0);
    assert(rr_db_byte_available(&paths[0]) == cap);
    fill(&paths[0], 3, 3, cap + 1, true);
    assert(byte_window_db(&paths[0], 1, 1) == 1); /* empty: oversized singleton */
    assert(!rr_db_byte_available(&paths[0]));
    complete(&paths[0], 3, 0);
    assert(rr_db_byte_available(&paths[0]) == cap);
    fini();
}

static void run_oversized_byte_window(void)
{
    const u32 cap = 32768;
    const u64 starts[] = {0, 65534, (1ULL << 48) - 2,
                         MLX5_SRM_REROUTE_MASK - 1};
    const u32 sizes[] = {cap + 1, 65536, UINT32_MAX};
    unsigned k, j;
    if (MLX5_SRM_MAX_INFLIGHT_BYTES != cap) return;
    for (k = 0; k < sizeof(starts) / sizeof(starts[0]); k++) {
        for (j = 0; j < sizeof(sizes) / sizeof(sizes[0]); j++) {
            struct mlx5_ib_srmc *s;
            u64 start = starts[k], initial = UINT64_MAX - 100;
            init(start); s = &paths[0];
            s->ctrl_page->route.posted_bytes = initial;
            s->ctrl_page->route.completed_bytes = initial;
            s->rr->completed_bytes = initial;
            fill(s, start, 7, sizes[j], true);
            fill(s, add(start, 1), 8, sizes[j], true);
            fill(s, add(start, 2), 9, 8192, true);
            assert(byte_window_db(s, 3, 0) == 0); /* no worker credit */
            assert(s->ctrl_page->route.posted_bytes == initial);
            assert(byte_window_db(s, 3, 3) == 1);
            assert(s->ctrl_page->route.posted_bytes - initial == sizes[j]);
            assert(byte_window_db(s, 2, 2) == 0); /* no second oversized WR */
            complete(s, start, 0);
            assert(rr_db_byte_available(s) == cap);
            assert(byte_window_db(s, 2, 2) == 1); /* no small tail in same DB */
            assert(byte_window_db(s, 1, 1) == 0);
            complete(s, add(start, 1), 0);
            assert(byte_window_db(s, 1, 1) == 1);
            complete(s, add(start, 2), 0);
            assert(s->ctrl_page->route.posted_bytes ==
                   s->ctrl_page->route.completed_bytes);
            fini();
        }
    }
    /* A normal prefix cannot share a DB with the following oversized WR. */
    init(0);
    fill(&paths[0], 0, 7, 8192, true);
    fill(&paths[0], 1, 8, 65536, true);
    assert(byte_window_db(&paths[0], 2, 2) == 1);
    assert(byte_window_db(&paths[0], 1, 1) == 0); /* some byte credit != empty */
    complete(&paths[0], 0, 0);
    assert(byte_window_db(&paths[0], 1, 1) == 1);
    complete(&paths[0], 1, 0);
    fini();
}

static void run_drain_byte_window(void)
{
    const u32 cap = 32768, payload = 8192;
    const u64 starts[] = {0, 65534, (1ULL << 48) - 2,
                         MLX5_SRM_REROUTE_MASK - 1};
    unsigned long saved_threshold = srm_reroute_copy_threshold_bytes;
    unsigned k, i;

    if (MLX5_SRM_MAX_INFLIGHT_BYTES != cap) return;
    srm_reroute_copy_threshold_bytes = 2UL * cap;
    for (k = 0; k < sizeof(starts) / sizeof(starts[0]); k++) {
        struct mlx5_srm_rr_group *g;
        struct mlx5_ib_srmc *s, *d;
        u64 start = starts[k];

        init(start); g = &sched.rr_groups[0]; s = &paths[0]; d = &paths[2];
        for (i = 0; i < 7; i++) fill(s, add(start, i), 7, payload, true);
        /* Inject an oversized WR into the frozen suffix to verify that
         * drain retains its exemption, including multi-WQE DB batches. */
        fill(s, add(start, 5), 7, cap + 1, true);
        s->ctrl_page->resv_idx = add(start, 7);
        assert(byte_window_db(s, 7, 7) == 4); /* active source obeys cap */
        assert(rr_db_byte_available(s) == 0);
        assert(rr_begin(g, 0)); rr_step(g);
        assert(!g->copy && g->phase == RR_OLD_DRAIN);
        assert(s->ctrl_page->route.state == MLX5_SRM_ROUTE_DRAIN);
        assert(!s->ctrl_page->route.user_db);
        assert(rr_db_byte_available(s) == UINT64_MAX);
        assert(rr_db_byte_available(d) == cap); /* no target exemption */
        assert(!mlx5_srm_rr_can_db(s, g->freeze_end));

        /* Target prewrites do not count as the frozen source suffix. */
        fill(d, start, 9, cap, true);
        fill(d, add(start, 1), 9, cap, true);
        d->ctrl_page->resv_idx = add(start, 2);
        assert(!mlx5_srm_rr_can_db(d, start));
        assert(byte_window_db(s, 3, 1) == 1); /* worker credit still clips */
        assert(s->ctrl_page->route.posted_bytes == cap + payload);
        assert(rr_schedule(&sched, 0, ktime_get_ns()) == 0);
        assert(!g->drain_db_ready && !g->timing_reported);
        assert(byte_window_db(s, 2, 2) == 2); /* no source CQ needed */
        assert(s->ctrl_page->route.posted_bytes == 6ULL * payload + cap + 1);
        assert(s->cq_complete_idx == start && s->rr->completed_bytes == 0);
        assert(rr_schedule(&sched, 0, ktime_get_ns()) == 2);
        assert(g->drain_db_ready && !d->ctrl_page->route.user_db);
        check_timing(3, "timing_end=drain_ready");

        assert(mlx5_srm_rr_can_db(d, start)); /* independent target user */
        assert(byte_window_db(d, 2, 2) == 1);
        assert(rr_db_byte_available(d) == 0);
        assert(byte_window_db(d, 1, 1) == 0);
        complete(d, start, 0);
        assert(rr_db_byte_available(d) == cap);
        for (i = 0; i < 7; i++) complete(s, add(start, i), 0);
        rr_step(g);
        assert(g->phase == RR_IDLE);
        assert(s->ctrl_page->route.state == MLX5_SRM_ROUTE_SPARE);
        assert(s->ctrl_page->route.posted_bytes == 6ULL * payload + cap + 1);
        assert(s->ctrl_page->route.completed_bytes == 6ULL * payload + cap + 1);
        assert(rr_db_byte_available(s) == cap); /* reuse restores cap */
        assert(d->ctrl_page->route.user_db);
        assert(byte_window_db(d, 1, 1) == 1);
        complete(d, add(start, 1), 0);
        fini();
    }
    srm_reroute_copy_threshold_bytes = saved_threshold;
}

int main(void)
{
    unsigned i,p;
    /* Test fixtures must not depend on the experiment's editable defaults. */
    srm_reroute_copy_threshold_bytes = 10240;
    srm_reroute_ratio_gap = 200;
    srm_reroute_interval_us = 1000;
    srm_reroute_cooldown_us = 1000;
    srm_reroute_bad_windows = 3;
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
    run_normal_fast_path();run_db_byte_prefix();run_byte_window();
    run_oversized_byte_window();
    run_drain_byte_window();
    printf("PASS: byte window=%u, exact/oversize/disabled, per-QP isolation, partial grant, shared completion refund, cumulative counter wrap\n",
           (unsigned int)MLX5_SRM_MAX_INFLIGHT_BYTES);
    if (MLX5_SRM_MAX_INFLIGHT_BYTES == 32768)
        puts("PASS: frozen drain source bypasses byte cap without CQ progress; partial worker grant, target cap, source accounting/reuse, frozen-WQE timing and sequence wrap");
    run_empty_source_prewrite();
    run_phase_budget_boundary();
    {
        const u64 starts[] = {0, 65534, (1ULL << 48) - 2,
                             MLX5_SRM_REROUTE_MASK - 1};
        for (i = 0; i < sizeof(starts) / sizeof(starts[0]); i++)
            run_drain_dependencies(starts[i]);
    }
    run_drain_snapshot_budget();
    for (i = 0; i < 2; i++) {
        u64 no_delay = run_work_timing(i, 0);
        assert(no_delay == run_work_timing(i, 1000000000ULL));
    }
#if MLX5_SRM_ENABLE_PRIVATE_CQ && MLX5_SRM_ENABLE_CQE_SIMPLIFY
    run_private_idle_batch();
#endif
    for(p=0;p<513;p++) free(dev.sq_ctrl_pool.pages[p]);
    free(dev.sq_ctrl_pool.pages);
    printf("PASS: 240 classified + 4800 empty migrations; budgets, owner contention, error mapping, detector, NOP retirement, 16/48/63-bit wrap; idle fast path, private completion counters, native token reads, partial DB byte accounting; blocked-WQE timing (copy, drain, empty source with prewrites, zero denominator, wrap); same-visit phase chaining, cross-phase budget, work timing excludes injected log/other-work delays; early drain DB (issued + unissued dependencies, partial DB gate, contiguous prefix, early completions, error, wrap), drain-ready timing, threshold=1 empty/nonempty classification; simplify=%d\n",
           MLX5_SRM_ENABLE_CQE_SIMPLIFY);
    return 0;
}
