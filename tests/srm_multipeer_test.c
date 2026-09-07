/* Hardware-independent tests; test_srm_multipeer.py injects real setup code. */
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "srm_kqp_layout.h"

typedef uint32_t u32;
#define NUM_SRMC 1024
#define SQ_DEPTH 35000
#define GFP_KERNEL 0
#define SRMC_CREATE_FLAG_INIT_QP 1
#define MESSAGE_SIZE_SMALL 0
#define MESSAGE_SIZE_LARGE 1
#define READ_ONCE(v) __atomic_load_n(&(v), __ATOMIC_ACQUIRE)
#define WRITE_ONCE(v, x) __atomic_store_n(&(v), (x), __ATOMIC_RELEASE)
#define smp_load_acquire(p) __atomic_load_n((p), __ATOMIC_ACQUIRE)
#define smp_store_release(p, x) __atomic_store_n((p), (x), __ATOMIC_RELEASE)
#define min_t(t, a, b) ((t)(a) < (t)(b) ? (t)(a) : (t)(b))
#define max_t(t, a, b) ((t)(a) > (t)(b) ? (t)(a) : (t)(b))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define unlikely(x) (x)
#define IS_ERR(x) false
#define pr_warn_ratelimited(...) ((void)0)
#define MLX5_SRM_KERNEL_QP_LEVELS levels
#define pr_info(...) ((void)0)
#define pr_err(...) ((void)0)
#define wake_up_all(p) ((void)(p))
#define mutex_lock(p) pthread_mutex_lock(p)
#define mutex_unlock(p) pthread_mutex_unlock(p)
#define kfree(p) free(p)
#define kvfree(p) free(p)
#define mlx5_ib_free_srmc_publish(p) ((void)(p))
#define kzalloc_node(n, flags, node) calloc(1, (n))
/* The test does not need the real metadata payload. */
#define mlx5_ib_srm_kvcalloc_node(n, size) calloc(1, 1)

union ib_gid { unsigned char raw[16]; };
struct mock_qp { struct { void *frags; } buf; };
static struct mock_qp mock_qps[NUM_SRMC];
struct mlx5_ib_srmc {
    union ib_gid dgid;
    struct { struct mock_qp *qp; int refcnt; } ini_cb;
    int *wqe_infos;
    int idx, srmc_idx;
    unsigned int owner_worker;
};
struct mlx5_ib_sched {
    pthread_mutex_t peer_lock, srmc_lock;
    struct mlx5_ib_srmc *srmc_tb[NUM_SRMC], *srmc_by_idx[NUM_SRMC];
    size_t srmc_cnt, ready_srmc_cnt;
    u32 kqp_slots, worker_count;
    int init_error, init_wait, id;
    void *workers;
};
static struct {
    struct mlx5_ib_sched *scheds;
    int num_sched;
    int usr_rc_routes[2048];
} sched_group;
struct mlx5_ib_dev { struct { u32 slot_cnt; } sq_ctrl_pool; };
struct ib_pd { struct mlx5_ib_dev *device; };
#define to_mdev(dev) (dev)
static unsigned int num_kqps, levels, calls, fail_call, inject_targets;
static struct mlx5_ib_sched *current_sched;
static u32 jhash(const void *p, size_t size, u32 seed)
{
    const unsigned char *bytes = p;
    size_t i;
    for (i = 0; i < size; i++)
        seed = seed * 33 + bytes[i];
    return seed;
}
static u32 jhash_2words(u32 a, u32 b, u32 seed) { return a * 33 + b + seed; }
static u32 mlx5_srm_effective_kqps(void) { return num_kqps * levels; }
static u32 mlx5_srm_kqp_owner(struct mlx5_ib_sched *sched, u32 slot)
{
    return mlx5_srm_layout_owner(slot, num_kqps, sched->worker_count);
}
static u32 sched_hash_ip(char *addr, unsigned int size)
{
    (void)addr;
    return size - 2; /* Force collisions and hash-table wraparound. */
}

static void add_target(void)
{
    unsigned int i;
    for (i = 0; i < NUM_SRMC; i++) {
        unsigned int j = (NUM_SRMC - 2 + i) % NUM_SRMC;
        if (!current_sched->srmc_tb[j]) {
            struct mlx5_ib_srmc *t = calloc(1, sizeof(*t));
            assert(t);
            t->srmc_idx = -1;
            t->dgid.raw[15] = 1;
            current_sched->srmc_tb[j] = t;
            current_sched->srmc_cnt++;
            return;
        }
    }
    assert(!"target table unexpectedly full");
}

static int create_srmc_qp_cm(struct mlx5_ib_srmc *srmc, struct ib_pd *pd,
                            union ib_gid *gid, int cls, int id, u32 depth)
{
    (void)pd; (void)gid; (void)id; (void)depth;
    calls++;
    assert(srmc->srmc_idx >= (int)current_sched->kqp_slots);
    assert(cls == (int)mlx5_srm_layout_large(srmc->srmc_idx,
                                             num_kqps, levels));
    assert(srmc->owner_worker < current_sched->worker_count);
    if (calls == fail_call)
        return -EIO;
    srmc->ini_cb.qp = &mock_qps[srmc->srmc_idx];
    srmc->ini_cb.qp->buf.frags = (void *)1;
    if (inject_targets)
        add_target(); /* A CM target may occupy the next bucket. */
    return 1;
}

#include "srm_setup_under_test.inc"
#include "srm_hot_ring_under_test.inc"
#include "srm_select_under_test.inc"

static void init_sched(struct mlx5_ib_sched *s, unsigned int lanes,
                       unsigned int nlevels, unsigned int workers)
{
    memset(s, 0, sizeof(*s));
    pthread_mutex_init(&s->peer_lock, NULL);
    pthread_mutex_init(&s->srmc_lock, NULL);
    s->worker_count = workers;
    s->workers = (void *)1;
    sched_group.scheds = s;
    sched_group.num_sched = 1;
    current_sched = s;
    num_kqps = lanes;
    levels = nlevels;
    calls = fail_call = inject_targets = 0;
}

static void cleanup(struct mlx5_ib_sched *s)
{
    size_t count = 0;
    unsigned int i;
    for (i = 0; i < NUM_SRMC; i++) {
        struct mlx5_ib_srmc *p = s->srmc_tb[i];
        if (p) {
            if (p->ini_cb.qp)
                assert(s->srmc_by_idx[p->srmc_idx] == p);
            free(p->wqe_infos);
            free(p);
            count++;
        }
    }
    assert(count == s->srmc_cnt); /* No target overwritten or leaked. */
    pthread_mutex_destroy(&s->peer_lock);
    pthread_mutex_destroy(&s->srmc_lock);
}

static void test_setup(unsigned int nlevels)
{
    struct mlx5_ib_sched s;
    struct mlx5_ib_dev dev = { .sq_ctrl_pool.slot_cnt = NUM_SRMC };
    struct ib_pd pd = { .device = &dev };
    union ib_gid a = {{0}}, b = {{0}}, c = {{0}};
    unsigned int size = 64 * nlevels, i;
    a.raw[15] = 1; b.raw[15] = 2; c.raw[15] = 3;
    init_sched(&s, 64, nlevels, 2);
    add_target(); /* Same GID, but no outgoing QP: not a reusable group. */
    inject_targets = 1;
    assert(is_xrc_exists(&s, &pd, &a, 1, 0, 64) == 1);
    assert(s.kqp_slots == size && s.ready_srmc_cnt == size);
    assert(is_xrc_exists(&s, &pd, &b, 1, 0, 64) == 1);
    assert(s.kqp_slots == 2 * size && calls == 2 * size);
    assert(is_xrc_exists(&s, &pd, &a, 1, 0, 64) == 0);
    assert(calls == 2 * size);
    for (i = 0; i < 2 * size; i++) {
        assert(s.srmc_by_idx[i]->srmc_idx == (int)i);
        assert(s.srmc_by_idx[i]->dgid.raw[15] == (i < size ? 1 : 2));
        assert(s.srmc_by_idx[i]->owner_worker ==
               mlx5_srm_layout_owner(i, 64, 2));
    }
    /* Exercise actual logical-QP selection, not just the allocation model. */
    for (i = 0; i < 1000; i++) {
        union ib_gid *gid = i % 2 ? &a : &b;
        struct mlx5_ib_srmc *selected =
            mlx5_ib_find_balanced_srmc_by_gid(gid, i);
        assert(selected && !memcmp(&selected->dgid, gid, sizeof(*gid)));
        assert(!mlx5_srm_layout_large(selected->srmc_idx, 64, nlevels));
        if (nlevels == 2) {
            struct mlx5_ib_srmc *large = s.srmc_by_idx[selected->srmc_idx + 64];
            assert(large && !memcmp(&large->dgid, gid, sizeof(*gid)));
            assert(large->owner_worker == selected->owner_worker);
        }
    }
    for (i = 0; i < 64; i++) {
        assert(s.srmc_by_idx[i]->ini_cb.refcnt >= 7 &&
               s.srmc_by_idx[i]->ini_cb.refcnt <= 8);
        assert(s.srmc_by_idx[size + i]->ini_cb.refcnt >= 7 &&
               s.srmc_by_idx[size + i]->ini_cb.refcnt <= 8);
    }
    assert(!mlx5_ib_find_balanced_srmc_by_gid(&c, 0));
    dev.sq_ctrl_pool.slot_cnt = 2 * size;
    assert(is_xrc_exists(&s, &pd, &c, 1, 0, 64) == -ENOSPC);
    assert(s.kqp_slots == 2 * size && calls == 2 * size);
    cleanup(&s);
}

static void test_partial_failure(void)
{
    struct mlx5_ib_sched s;
    struct mlx5_ib_dev dev = { .sq_ctrl_pool.slot_cnt = NUM_SRMC };
    struct ib_pd pd = { .device = &dev };
    union ib_gid a = {{1}}, b = {{2}};
    init_sched(&s, 4, 1, 1);
    assert(is_xrc_exists(&s, &pd, &a, 1, 0, 64) == 1);
    fail_call = calls + 3;
    assert(is_xrc_exists(&s, &pd, &b, 1, 0, 64) == -EIO);
    assert(s.kqp_slots == 4 && s.ready_srmc_cnt == 4);
    assert(s.srmc_by_idx[4] && s.srmc_by_idx[5]); /* teardown owns them */
    assert(!s.srmc_by_idx[6]);
    assert(is_xrc_exists(&s, &pd, &b, 1, 0, 64) == -EIO);
    assert(calls == fail_call); /* Never reuse the failed group's slots. */
    cleanup(&s);
}

struct setup_thread { struct mlx5_ib_sched *s; struct ib_pd *pd; union ib_gid gid; };
static void *setup_thread(void *arg)
{
    struct setup_thread *a = arg;
    int ret = is_xrc_exists(a->s, a->pd, &a->gid, 1, 0, 64);
    assert(ret == 0 || ret == 1);
    return NULL;
}

static void test_concurrent_setup(void)
{
    struct mlx5_ib_sched s;
    struct mlx5_ib_dev dev = { .sq_ctrl_pool.slot_cnt = NUM_SRMC };
    struct ib_pd pd = { .device = &dev };
    struct setup_thread args[12];
    pthread_t threads[12];
    unsigned int i;
    init_sched(&s, 5, 2, 3);
    for (i = 0; i < 12; i++) {
        args[i] = (struct setup_thread){ .s = &s, .pd = &pd };
        args[i].gid.raw[15] = i % 3 + 1;
        assert(!pthread_create(&threads[i], NULL, setup_thread, &args[i]));
    }
    for (i = 0; i < 12; i++)
        assert(!pthread_join(threads[i], NULL));
    assert(calls == 30 && s.kqp_slots == 30 && s.ready_srmc_cnt == 30);
    cleanup(&s);
}

static void test_rings(void)
{
    unsigned int next[NUM_SRMC], prev[NUM_SRMC], heads[2], counts[2];
    unsigned int lanes, nlevels, workers, w, peers, cls, step, slot;
    unsigned int seen[NUM_SRMC];
    for (lanes = 1; lanes <= 65; lanes++)
    for (nlevels = 1; nlevels <= 2; nlevels++)
    for (workers = 1; workers <= lanes; workers++)
    for (peers = 1; peers <= 3; peers++) {
        unsigned int slots = peers * lanes * nlevels;
        memset(seen, 0, sizeof(seen));
        for (w = 0; w < workers; w++) {
            unsigned int begin = lanes * w / workers;
            unsigned int end = lanes * (w + 1) / workers;
            mlx5_srm_layout_build_rings(next, prev, heads, counts,
                                        slots, lanes, nlevels, begin, end);
            assert(counts[0] == peers * (end - begin));
            assert(counts[1] == (nlevels - 1) * counts[0]);
            for (cls = 0; cls < nlevels; cls++) {
                /* Simulate a hot hint for the last peer, then a full pass. */
                unsigned int hot = (peers - 1) * lanes * nlevels + cls * lanes + begin;
                mlx5_srm_poll_ring_move_front(next, prev, &heads[cls], hot);
                assert(heads[cls] == hot);
                slot = heads[cls];
                for (step = 0; step < counts[cls]; step++) {
                    assert(slot < slots && !seen[slot]++);
                    assert(mlx5_srm_layout_owner(slot, lanes, workers) == w);
                    assert(mlx5_srm_layout_large(slot, lanes, nlevels) == cls);
                    assert(next[prev[slot]] == slot && prev[next[slot]] == slot);
                    slot = next[slot];
                }
                assert(slot == heads[cls]);
            }
        }
        for (slot = 0; slot < slots; slot++)
            assert(seen[slot] == 1);
    }
}

int main(void)
{
    test_setup(1);
    test_setup(2);
    test_partial_failure();
    test_concurrent_setup();
    test_rings();
    puts("PASS: multi-peer setup, reuse, collisions, capacity, failure, concurrent creation, worker/size/hot rings");
    return 0;
}
