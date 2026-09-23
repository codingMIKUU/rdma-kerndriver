/* Real poll_srmc_inline with a mocked per-CQ poll. Check that a blocked
 * software CQ / slow peer never prevents other private CQs from running. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
typedef uint64_t u64;
typedef uint8_t u8;
#define SRMC_POLLING_CNT 8
#define MLX5_SRM_CQ_INDEX(i) (i)
#define READ_ONCE(x) (x)
#define WARN_ON_ONCE(x) (x)
#define unlikely(x) (x)
struct mlx5_ib_srmc { int srmc_idx, sig_cnt, pending; bool blocked; };
struct mlx5_ib_sched { int unused; };
struct mlx5_qp_ctrl_pool { int unused; };
struct ib_wc { int unused; };
struct mlx5_srm_cq_workspace { int unused; };
struct mlx5_ib_srm_sched_stats { u64 cq_poll_calls; };
static unsigned srm_stats_sample_rate;
static bool srm_stats_enable;
static u64 ktime_get_ns(void) { return 0; }
static void mlx5_ib_srm_record_cq_inline_queue(
    struct mlx5_ib_srm_sched_stats *stats, u64 ns) { }
static int last, calls;
static int srm_poll_srmc_once(struct mlx5_ib_sched *sched,
    struct mlx5_qp_ctrl_pool *pool, struct mlx5_ib_srmc *s,
    struct ib_wc *wc, void **cqe, struct mlx5_srm_cq_workspace *ws,
    struct mlx5_ib_srm_sched_stats *stats)
{
    int done = s->pending && !s->blocked;
    last = s->srmc_idx; calls++;
    s->pending -= done;
    s->sig_cnt = s->pending;
    return done;
}
static __always_inline int poll_srmc_inline(struct mlx5_ib_sched *,
    struct mlx5_qp_ctrl_pool *, struct mlx5_ib_srmc **, int *, int *, u8 *,
    struct ib_wc *, void **, struct mlx5_srm_cq_workspace *, int,
    struct mlx5_ib_srm_sched_stats *);
int main(void)
{
    /* Distinct peer/size slots that would all alias in the old shared CQ. */
    struct mlx5_ib_srmc s[3] = {
        {.srmc_idx=0, .pending=2, .blocked=true},
        {.srmc_idx=63, .pending=2}, {.srmc_idx=127, .pending=2}};
    struct mlx5_ib_srmc *ring[SRMC_POLLING_CNT] = {&s[0], &s[1], &s[2]};
    u8 in_queue[128] = {[0]=1, [63]=1, [127]=1};
    int tail = 0, head = 3;
#define POLL() poll_srmc_inline(NULL, NULL, ring, &tail, &head, in_queue, \
                               NULL, NULL, NULL, 0, NULL)
    for (int i = 0; i < 6; i++) {
        int before = calls;
        assert(POLL() == (i % 3 ? 1 : 0));
        assert(calls == before+1 && last == s[i % 3].srmc_idx);
    }
    assert(in_queue[0] && !in_queue[63] && !in_queue[127]);
    assert(s[0].pending == 2 && !s[1].pending && !s[2].pending);
    s[0].blocked = false;
    assert(POLL() == 1 && in_queue[0]);
    assert(POLL() == 1 && !in_queue[0]);
    assert(tail == head && POLL() == 0);
    puts("PASS: private CQ round-robin, blocked destination, global slot IDs and empty ring");
    return 0;
}
