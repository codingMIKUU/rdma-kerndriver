/* The runner appends the production non-simplified scheduler poll function.
 * Mock the hardware poll and user-CQ publisher, not the dispatch control flow. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint64_t u64;
typedef uint32_t u32;
#define SQ_DEPTH 16
#define SRM_CTRL_COMPLETE_CACHE 32
#define SRM_CQE_PUBLISH_BATCH 4
#define min_t(t,a,b) ((t)(a) < (t)(b) ? (t)(a) : (t)(b))
#define likely(x) (x)
#define DEBUG_LOG(...) ((void)0)
#define jiffies 1
struct ib_wc { u64 wr_id; };
struct mlx5_ib_cqbuf { int id; };
struct mlx5_qp_ctrl_pool { int unused; };
struct mlx5_ib_srm_sched_stats { int unused; };
struct mlx5_ib_srmc {
    int sig_cnt;
    unsigned long last_cqe_jiffies;
    struct { void *cq; } ini_cb;
};
struct mock_credit { u64 completed_total; };
struct mlx5_ib_sched_worker { struct mock_credit *credit_ctrl; };
struct mlx5_ib_sched { struct mlx5_ib_sched_worker *worker; };
struct mlx5_srm_ctrl_complete_entry { int unused; };
struct mlx5_srm_cqe_publish_entry {
    struct mlx5_ib_cqbuf *cqb;
    void *cqe64;
    u64 wqe_counter;
    struct mlx5_ib_srmc *srmc;
};
struct mlx5_srm_cqe_publish_lane {
    struct mlx5_ib_cqbuf *cqb;
    int count;
    struct mlx5_srm_cqe_publish_entry entries[SRM_CQE_PUBLISH_BATCH];
};
struct mlx5_srm_cq_workspace { struct mlx5_srm_cqe_publish_lane publish_lane; };
static int ready, polled, published, calls, last_budget, inject_error;
static u64 db_tail, hw_cursor, recycled;
static struct { int status; } raw[16];
static struct mlx5_ib_cqbuf user_cqs[2];
static struct mlx5_ib_sched_worker *mlx5_srm_cq_worker(
    struct mlx5_ib_sched *sched, struct mlx5_ib_srmc *s)
{ (void)s; return sched->worker; }
static int mlx5_srm_refresh_poll_budget(struct mlx5_ib_sched *sched,
                                        struct mlx5_ib_srmc *s)
{ (void)sched; return s->sig_cnt = (int)(db_tail - hw_cursor); }
static int mlx5_ib_poll_cq_with_cqe(void *cq, int budget, struct ib_wc *wc,
                                   void **cqe, u32 *completed)
{
    int n = 0;
    (void)cq;
    calls++; last_budget = budget; *completed = 0;
    assert(budget > 0 && budget <= SQ_DEPTH);
    if (inject_error) return -5;
    while (n < budget && polled < ready) {
        /* Each signaled CQE covers two WRs; credit != CQE count. */
        hw_cursor += 2;
        wc[n].wr_id = hw_cursor - 1;
        cqe[n++] = &raw[polled++];
        *completed += 2;
    }
    return n;
}
static void mlx5_srm_cq_workspace_reset(struct mlx5_srm_cq_workspace *ws)
{ memset(ws, 0, sizeof(*ws)); }
static bool mlx5_srm_resolve_cqe_ctx(struct mlx5_ib_sched *sched,
    struct mlx5_qp_ctrl_pool *pool, u64 wrid, struct mlx5_srm_cq_workspace *ws,
    struct mlx5_srm_cqe_publish_entry *e)
{
    (void)sched; (void)pool; (void)ws;
    e->wqe_counter = wrid;
    e->cqb = &user_cqs[(wrid / 2) % 2];
    e->srmc = NULL;
    return true;
}
static void mlx5_srm_complete_wrid_ctrl(struct mlx5_qp_ctrl_pool *p, u64 wrid,
    struct mlx5_srm_ctrl_complete_entry *c, int *n)
{ (void)p; (void)wrid; (void)c; (void)n; assert(!"unexpected route miss"); }
static void mlx5_srm_flush_cqe_publish_lane(struct mlx5_ib_srmc *s,
    struct mlx5_srm_cqe_publish_lane *lane,
    struct mlx5_srm_ctrl_complete_entry *cache, int *cnt,
    struct mlx5_ib_srm_sched_stats *stats)
{
    int i;
    (void)s; (void)cache; (void)cnt; (void)stats;
    for (i = 0; i < lane->count; ++i) {
        assert(lane->entries[i].wqe_counter == (u64)(2 * published + 1));
        assert(lane->entries[i].cqe64 == &raw[published]);
        assert(lane->entries[i].cqb == &user_cqs[published % 2]);
        published++;
    }
    lane->count = 0;
}
static struct mlx5_srm_cqe_publish_lane *mlx5_srm_get_publish_lane(
    struct mlx5_ib_srmc *s, struct mlx5_srm_cq_workspace *ws,
    struct mlx5_ib_cqbuf *cqb, struct mlx5_srm_ctrl_complete_entry *c,
    int *n, struct mlx5_ib_srm_sched_stats *stats)
{
    if (ws->publish_lane.cqb != cqb)
        mlx5_srm_flush_cqe_publish_lane(s, &ws->publish_lane, c, n, stats);
    ws->publish_lane.cqb = cqb;
    return &ws->publish_lane;
}
static void mlx5_srm_ctrl_complete_flush(struct mlx5_srm_ctrl_complete_entry *c,
                                         int *n)
{ (void)c; (void)n; assert(published == polled); recycled = hw_cursor; }
static void atomic64_add(u32 n, u64 *completed)
{
    assert(published == polled && recycled == hw_cursor);
    *completed += n;
    assert(*completed == hw_cursor);
}
#define mlx5_ib_srm_begin_cq_timing(s) ((void)(s), false)
#define mlx5_ib_srm_cq_timing_active(s) ((void)(s), false)
#define ktime_get_ns() 0ULL
#define mlx5_ib_srm_record_cq_phase(s,p,t) ((void)(s), (void)(t))
#define mlx5_ib_srm_record_timed_cqes(s,n) ((void)(s), (void)(n))
#define mlx5_ib_srm_record_cq_once(s,t) ((void)(s), (void)(t))
#define mlx5_ib_srm_record_cq_post_poll(s,t) ((void)(s), (void)(t))
#define mlx5_ib_srm_end_cq_timing(s) ((void)(s))
#define rdtsc_ordered() 1234ULL
#define mlx5_srm_timing_publish_kernel_cqe(s,p,t) ((void)(s), (void)(p), (void)(t))
static inline int srm_poll_srmc_once(struct mlx5_ib_sched *,
    struct mlx5_qp_ctrl_pool *, struct mlx5_ib_srmc *, struct ib_wc *, void **,
    struct mlx5_srm_cq_workspace *, struct mlx5_ib_srm_sched_stats *);

int main(void)
{
    struct mock_credit credit = {0};
    struct mlx5_ib_sched_worker worker = {&credit};
    struct mlx5_ib_sched sched = {&worker};
    struct mlx5_ib_srmc s = {0};
    struct mlx5_srm_cq_workspace ws;
    struct ib_wc wc[SQ_DEPTH];
    void *cqe[SQ_DEPTH];
    int before, ret;
#define POLL() srm_poll_srmc_once(&sched, NULL, &s, wc, cqe, &ws, NULL)
    assert(POLL() == -1 && calls == 0);
    db_tail = 100; /* User DB discovered despite the initial sig_cnt == 0. */
    assert(POLL() == 0 && calls == 1 && s.sig_cnt == 100);
    assert(last_budget == min_t(int, MLX5_SRM_PRIVATE_CQ_POLL_BUDGET, SQ_DEPTH));
    inject_error = 1;
    assert(POLL() == -5 && credit.completed_total == 0 && recycled == 0);
    inject_error = 0;
    ready = 8; raw[3].status = 13; /* Native error CQE must be dispatched too. */
    while (polled < ready) {
        before = polled;
        ret = POLL();
        assert(ret == min_t(int, ready - before,
                           min_t(int, MLX5_SRM_PRIVATE_CQ_POLL_BUDGET, SQ_DEPTH)));
        assert(published == polled && credit.completed_total == (u64)polled * 2);
        assert(s.sig_cnt == 100 - polled * 2);
    }
    assert(POLL() == 0 && published == 8);
    db_tail = hw_cursor; before = calls;
    assert(POLL() == -1 && calls == before);
    db_tail += 2; ready++; /* A later user DB reactivates the CQ. */
    assert(POLL() == 1 && s.sig_cnt == 0 && published == 9);
    printf("PASS private CQ native dispatch: budget=%d timing=%d\n",
           MLX5_SRM_PRIVATE_CQ_POLL_BUDGET, MLX5_SRM_ENABLE_WQE_TIMING);
    return 0;
}
