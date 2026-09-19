/* The runner appends the production priority selector and poll_srmc_inline.
 * Exercise priority ordering without hardware; normal CQ ring is real code. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;
#define MLX5_SRM_CQ_SLOTS 8
#define SRMC_POLLING_CNT 16
#define MLX5_SRM_CQ_INDEX(i) (i)
#define READ_ONCE(x) (x)
#define WARN_ON_ONCE(x) (x)
#define unlikely(x) (x)
struct mlx5_ib_srmc { int srmc_idx, sig_cnt; u32 owner_worker; int pending; };
struct mlx5_ib_sched_worker {
    unsigned long latency_cqs[1];
    u32 latency_cq_count, latency_cq_cursor, worker_id;
};
struct mlx5_ib_sched { struct mlx5_ib_srmc *srmc_by_idx[MLX5_SRM_CQ_SLOTS]; };
struct mlx5_qp_ctrl_pool { int unused; };
struct ib_wc { int unused; };
struct mlx5_srm_cq_workspace { struct mlx5_ib_sched_worker *latency_worker; };
struct mlx5_ib_srm_sched_stats { u64 cq_poll_calls; };
static unsigned int srm_stats_sample_rate;
static bool srm_stats_enable;
static u64 ktime_get_ns(void) { return 0; }
static void mlx5_ib_srm_record_cq_inline_queue(
    struct mlx5_ib_srm_sched_stats *stats, u64 cycles)
{ (void)stats; (void)cycles; }
#if MLX5_SRM_ENABLE_LATENCY_CQ_PRIORITY
static unsigned long find_next_bit(const unsigned long *b, unsigned long n,
                                   unsigned long start)
{
    for (; start < n; ++start)
        if (*b & (1UL << start)) return start;
    return n;
}
#define find_first_bit(b, n) find_next_bit(b, n, 0)
#endif
static int order[256], nr_calls;
static int srm_poll_srmc_once(struct mlx5_ib_sched *sched,
    struct mlx5_qp_ctrl_pool *pool, struct mlx5_ib_srmc *s,
    struct ib_wc *wc, void **cqe, struct mlx5_srm_cq_workspace *workspace,
    struct mlx5_ib_srm_sched_stats *stats)
{
    (void)sched; (void)pool; (void)wc; (void)cqe; (void)workspace; (void)stats;
    assert(nr_calls < 256);
    order[nr_calls++] = s->srmc_idx;
    /* A bounded batch of one, refreshing outstanding from user DB. */
    if (s->pending) {
        s->sig_cnt = --s->pending;
        return 1;
    }
    s->sig_cnt = 0;
    return 0;
}
static __always_inline int poll_srmc_inline(struct mlx5_ib_sched *,
    struct mlx5_qp_ctrl_pool *, struct mlx5_ib_srmc **, int *, int *, u8 *,
    struct ib_wc *, void **, struct mlx5_srm_cq_workspace *, int,
    struct mlx5_ib_srm_sched_stats *);

int main(void)
{
    struct mlx5_ib_srmc s[3] = {{.srmc_idx=0}, {.srmc_idx=1}, {.srmc_idx=2}};
    struct mlx5_ib_sched sched = {.srmc_by_idx={&s[0], &s[1], &s[2]}};
    struct mlx5_ib_sched_worker w = {0};
    struct mlx5_srm_cq_workspace ws = {.latency_worker=&w};
    struct mlx5_ib_srmc *ring[SRMC_POLLING_CNT] = {&s[0], &s[2]};
    u8 in_queue[MLX5_SRM_CQ_SLOTS] = {1, 0, 1};
    int tail = 0, head = 2, ret;
    int i;
#define POLL() poll_srmc_inline(&sched, NULL, ring, &tail, &head, in_queue, \
                              NULL, NULL, &ws, 0, NULL)
    w.latency_cq_count = 1;
    w.latency_cqs[0] = 1UL << 1;
    s[0].pending = s[1].pending = s[2].pending = 100;
    for (i = 0; i < 8; i++) {
        nr_calls = 0;
        ret = POLL();
#if MLX5_SRM_ENABLE_LATENCY_CQ_PRIORITY
        assert(ret == 2 && nr_calls == 2 && order[0] == 1);
        assert(order[1] == (i % 2 ? 2 : 0));
#else
        assert(ret == 1 && nr_calls == 1);
        assert(order[0] == (i % 2 ? 2 : 0));
#endif
        assert(in_queue[0] && !in_queue[1] && in_queue[2]);
    }
#if MLX5_SRM_ENABLE_LATENCY_CQ_PRIORITY
    /* Same CQ at normal ring head: do not poll it twice. */
    w.latency_cqs[0] = 1UL << ring[tail]->srmc_idx;
    w.latency_cq_cursor = 0;
    nr_calls = 0;
    assert(POLL() == 1 && nr_calls == 1);
    /* Empty ordinary ring: user DB on a tagged CQ is still discovered. */
    memset(ring, 0, sizeof(ring));
    memset(in_queue, 0, sizeof(in_queue));
    head = tail = 0;
    w.latency_cqs[0] = 1UL << 1;
    s[1].sig_cnt = 0;
    s[1].pending = 1;
    assert(POLL() == 1 && s[1].pending == 0 && head == tail);
    assert(!in_queue[1]);
    assert(POLL() == 0); /* empty priority CQ never waits */
    /* Split/reroute candidates rotate, including wrap at end of bitmap. */
    w.latency_cq_count = 2;
    w.latency_cqs[0] = (1UL << 1) | (1UL << 2);
    w.latency_cq_cursor = MLX5_SRM_CQ_SLOTS;
    nr_calls = 0;
    POLL(); POLL(); POLL();
    assert(nr_calls == 3 && order[0] == 1 && order[1] == 2 && order[2] == 1);
    /* A disappearing hint and another worker's CQ are safe to skip. */
    w.latency_cqs[0] = 0;
    nr_calls = 0;
    assert(POLL() == 0 && nr_calls == 0);
    w.latency_cqs[0] = 1UL << 1;
    s[1].owner_worker = 1;
    assert(POLL() == 0 && nr_calls == 0);
    w.latency_cq_count = 0;
    assert(POLL() == 0 && nr_calls == 0);
#endif
    printf("PASS: latency CQ priority=%d ordering, fairness, empty ring and lifecycle hints\n",
           MLX5_SRM_ENABLE_LATENCY_CQ_PRIORITY);
    return 0;
}
