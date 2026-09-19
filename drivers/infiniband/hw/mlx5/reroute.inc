/* SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
 * Included by scheduler.c: migration is serviced by its sole CQ/DB worker.
 * No syscalls, blocking waits, allocations or locks spanning scheduler turns.
 */
#if MLX5_SRM_ENABLE_REROUTE
static bool srm_reroute_enable=true;
module_param(srm_reroute_enable, bool, 0644);
static unsigned int srm_reroute_interval_us = 1000;
module_param(srm_reroute_interval_us, uint, 0644);
MODULE_PARM_DESC(srm_reroute_interval_us,
                 "Multi-KQP reroute detector interval in microseconds");
static unsigned int srm_reroute_ratio_gap = 1;
module_param(srm_reroute_ratio_gap, uint, 0644);
static unsigned int srm_reroute_bad_windows = 3;
module_param(srm_reroute_bad_windows, uint, 0644);
static unsigned int srm_reroute_cooldown_us = 1000;
module_param(srm_reroute_cooldown_us, uint, 0644);
MODULE_PARM_DESC(srm_reroute_cooldown_us,
                 "Minimum microseconds between multi-KQP reroutes");
/* Editable default, in bytes: total payload of ALL frozen, un-DB'd WQEs.
 * >= threshold copies to the new SQ; < threshold drains the old SQ. */
static unsigned long srm_reroute_copy_threshold_bytes = 1000UL * 1024;
module_param(srm_reroute_copy_threshold_bytes, ulong, 0644);
static unsigned int srm_reroute_warn_ms = 1000;
module_param(srm_reroute_warn_ms, uint, 0644);
/* Diagnose one logical group at a time without logging every 10 ms check. */
static bool srm_reroute_detect_log_enable=1;
module_param(srm_reroute_detect_log_enable, bool, 0644);
static unsigned int srm_reroute_detect_log_interval_ms = 5000;
module_param(srm_reroute_detect_log_interval_ms, uint, 0644);
static int srm_reroute_detect_log_group;
module_param(srm_reroute_detect_log_group, int, 0644);
/* Explicit one-shot test hook: physical source index, -1 disables it. */
static int srm_reroute_test_source = -1;
module_param(srm_reroute_test_source, int, 0644);

enum rr_phase { RR_IDLE, RR_READY, RR_SNAPSHOT, RR_RESERVE,
                RR_COPY0, RR_COPY1, RR_COPY_DRAIN, RR_OLD_DRAIN, RR_RECYCLE,
                RR_NOP_WAIT, RR_FAILED };
struct rr_meta {
    struct mlx5_ib_srmc *origin;
    u64 post;
};
struct mlx5_srm_rr_path {
    struct rr_meta *meta;
    unsigned long *done;
    struct mlx5_srm_rr_group *group;
    u64 logical_cons;
    /* Single CQ/scheduler worker: no user DB writer shares this counter. */
    u64 completed_bytes;
    struct mlx5_ib_srmc *publish_next;
    bool publish_pending;
    bool maintenance;
    u64 maintenance_end;
};
struct mlx5_srm_rr_group {
    struct mlx5_ib_srmc *paths[MLX5_SRM_REROUTE_PATHS];
    struct mlx5_ib_srmc *src, *dst;
    unsigned long *dependent;
    u64 *dep_end;
    u64 db_stop, freeze_end, cursor, bytes, dst_begin, dst_end, copied;
    u64 timing_start_cycles, timing_prewrite_cycles;
    u64 last_posted[2], last_completed[2];
    u64 next_check_ns, last_switch_ns;
    unsigned long next_detect_log, last_warn, started;
    u32 bad[2], source_slot, outstanding;
    enum rr_phase phase;
    u32 id;
    bool copy, copying_done, timing_reported;
};

static void rr_report_kernel_db_eligible(struct mlx5_srm_rr_group *g)
{
    u64 now;
    u64 switch_to_prewrite;
    u64 prewrite_to_kernel_db;
    u64 total;

    if (g->timing_reported || !g->timing_start_cycles ||
        !g->timing_prewrite_cycles)
        return;

    now = rdtsc_ordered();
    switch_to_prewrite = g->timing_prewrite_cycles -
                         g->timing_start_cycles;
    prewrite_to_kernel_db = now - g->timing_prewrite_cycles;
    total = switch_to_prewrite + prewrite_to_kernel_db;
    g->timing_reported = true;
    pr_info("SRM_REROUTE_TIMING group=%u src=%d dst=%d mode=%s switch_to_user_prewrite_cycles=%llu user_prewrite_to_kernel_db_cycles=%llu total_cycles=%llu\n",
            g->id, g->src ? g->src->srmc_idx : -1,
            g->dst ? g->dst->srmc_idx : -1,
            g->copy ? "copy" : "drain", switch_to_prewrite,
            prewrite_to_kernel_db, total);
}

static void rr_log(struct mlx5_srm_rr_group *g, const char *event)
{
    pr_info("SRM_REROUTE event=%s group=%u src=%d dst=%d db_stop=%llu freeze_end=%llu wqes=%llu bytes=%llu elapsed_ms=%u\n",
        event, g->id, g->src ? g->src->srmc_idx : -1,
        g->dst ? g->dst->srmc_idx : -1, g->db_stop, g->freeze_end,
        mlx5_srm_rr_seq(g->freeze_end - g->db_stop), g->bytes,
        jiffies_to_msecs(jiffies - g->started));
}

static void rr_warn(struct mlx5_srm_rr_group *g, const char *event)
{
    if (time_after_eq(jiffies, g->last_warn +
        msecs_to_jiffies(max(1U, READ_ONCE(srm_reroute_warn_ms))))) {
        g->last_warn = jiffies;
        rr_log(g, event);
    }
}

static void rr_detect_log(struct mlx5_srm_rr_group *g,
                          struct mlx5_srm_route_ctrl *route,
                          const u64 posted[2], const u64 completed[2],
                          const u64 ratio[2], u64 now_ns)
{
    struct mlx5_ib_qp *qp0, *qp1;
    u32 path0, path1;
    bool cooldown;
    u64 cooldown_ns;
    int selected_group = READ_ONCE(srm_reroute_detect_log_group);

    if (likely(!READ_ONCE(srm_reroute_detect_log_enable)) ||
        selected_group < 0 || (u32)selected_group != g->id ||
        time_before(jiffies, g->next_detect_log))
        return;

    g->next_detect_log = jiffies + msecs_to_jiffies(max(1000U,
        READ_ONCE(srm_reroute_detect_log_interval_ms)));
    path0 = READ_ONCE(route->active[0]);
    path1 = READ_ONCE(route->active[1]);
    if (path0 >= MLX5_SRM_REROUTE_PATHS ||
        path1 >= MLX5_SRM_REROUTE_PATHS)
        return;
    qp0 = g->paths[path0] ? g->paths[path0]->ini_cb.qp : NULL;
    qp1 = g->paths[path1] ? g->paths[path1]->ini_cb.qp : NULL;
    cooldown_ns = (u64)READ_ONCE(srm_reroute_cooldown_us) *
                  NSEC_PER_USEC;
    cooldown = g->last_switch_ns &&
               now_ns - g->last_switch_ns < cooldown_ns;
    pr_info("SRM_REROUTE_DETECT group=%u scope=last_detect_window "
            "active_kqp=%u,%u qpn=%u,%u posted_bytes=%llu,%llu "
            "completed_bytes=%llu,%llu ratio_permille=%llu,%llu "
            "gap=%llu gap_threshold=%u bad_before=%u,%u "
            "bad_required=%u cooldown=%u enabled=%u\n",
            g->id, g->paths[path0] ? g->paths[path0]->srmc_idx : U32_MAX,
            g->paths[path1] ? g->paths[path1]->srmc_idx : U32_MAX,
            qp0 ? qp0->ibqp.qp_num : 0, qp1 ? qp1->ibqp.qp_num : 0,
            posted[0], posted[1], completed[0], completed[1],
            ratio[0], ratio[1],
            ratio[0] > ratio[1] ? ratio[0] - ratio[1] : ratio[1] - ratio[0],
            READ_ONCE(srm_reroute_ratio_gap), g->bad[0], g->bad[1],
            max(1U, READ_ONCE(srm_reroute_bad_windows)), cooldown,
            READ_ONCE(srm_reroute_enable));
}

static void *rr_wqe(struct mlx5_ib_srmc *s, u64 post)
{
    return mlx5_frag_buf_get_wqe(&s->ini_cb.qp->sq.fbc,
                         post & (s->ini_cb.qp->sq.wqe_cnt - 1));
}

static bool rr_ready(struct mlx5_ib_srmc *s, u64 post)
{
    u64 token = mlx5_ib_srmc_get_publish_token(s, post);
    return mlx5_srm_publish_seq(token) ==
        ((post + 1) & MLX5_SRM_PUBLISH_SEQ_MASK);
}

static void rr_token(struct mlx5_ib_srmc *s, u64 post, u16 user)
{
    size_t off = (post & (s->publish_depth - 1)) * sizeof(u64);
    u64 *p = page_address(s->publish_pages[off / PAGE_SIZE]) + off % PAGE_SIZE;
    smp_store_release(p, (((post + 1) & MLX5_SRM_PUBLISH_SEQ_MASK) << 16) | user);
}

static u64 rr_descriptor_bytes(const void *wqe)
{
    /* V1 preflight accepts one non-inline SGE, XRC READ/WRITE (four DS). */
    const struct mlx5_wqe_data_seg *d = wqe + 48;
    return be32_to_cpu(READ_ONCE(d->byte_count));
}

static u64 rr_bytes(struct mlx5_ib_srmc *s, u64 post)
{
    return rr_descriptor_bytes(rr_wqe(s, post));
}

/* The scan owns db_owner but does not reserve worker credit. If another
 * producer consumes credit, account only the prefix actually granted. Read
 * the shorter of that prefix and the discarded suffix, BEFORE ringing DB.
 * Full grants (the common case) do not touch descriptors again. */
static u64 rr_db_prefix_bytes(struct mlx5_ib_srmc *s, u64 start,
                              u32 scanned, u32 granted, u64 scanned_bytes)
{
    u32 n;
    u64 bytes = 0;

    if (!granted) return 0;
    if (granted == scanned) return scanned_bytes;
    if (granted <= scanned - granted) {
        for (n = 0; n < granted; n++) bytes += rr_bytes(s, start + n);
        return bytes;
    }
    for (n = granted; n < scanned; n++)
        scanned_bytes -= rr_bytes(s, start + n);
    return scanned_bytes;
}

#if MLX5_SRM_ENABLE_CQE_SIMPLIFY
static struct mlx5_srm_migration_completion *
rr_record(struct mlx5_ib_srmc *s, u16 user)
{
    struct mlx5_qp_ctrl_pool *pool =
        &to_mdev(s->ini_cb.qp->ibqp.device)->sq_ctrl_pool;
    size_t off = (size_t)NUM_SRMC * 512 +
        user * sizeof(struct mlx5_srm_migration_completion);
    return page_address(pool->pages[off / PAGE_SIZE]) + off % PAGE_SIZE;
}
#endif

static int rr_sched_init(struct mlx5_ib_sched *sched)
{
    int g;
    sched->rr_groups = kvcalloc(num_kqps, sizeof(*sched->rr_groups), GFP_KERNEL);
    if (!sched->rr_groups) return -ENOMEM;
    for (g = 0; g < num_kqps; g++) {
        struct mlx5_srm_rr_group *r = &sched->rr_groups[g];
        r->id = g;
        r->dependent = bitmap_zalloc(MLX5_SRM_REROUTE_USERS, GFP_KERNEL);
        r->dep_end = kvcalloc(MLX5_SRM_REROUTE_USERS, sizeof(u64), GFP_KERNEL);
        if (!r->dependent || !r->dep_end) return -ENOMEM;
    }
    return 0;
}

static void rr_sched_free(struct mlx5_ib_sched *sched)
{
    int g;
    if (!sched->rr_groups) return;
    for (g = 0; g < num_kqps; g++) {
        bitmap_free(sched->rr_groups[g].dependent);
        kvfree(sched->rr_groups[g].dep_end);
    }
    kvfree(sched->rr_groups);
    sched->rr_groups = NULL;
}

static int rr_path_init(struct mlx5_ib_sched *sched, struct mlx5_ib_srmc *s)
{
    struct mlx5_srm_rr_path *r;
    struct mlx5_srm_rr_group *g;
    struct mlx5_srm_route_ctrl *c = &s->ctrl_page->route;
    unsigned int p = s->srmc_idx % MLX5_SRM_REROUTE_PATHS;
    if (s->ini_cb.qp->sq.wqe_cnt > 65536) {
        pr_err("SRM reroute SQ depth %u exceeds unambiguous 16-bit counter window\n",
               s->ini_cb.qp->sq.wqe_cnt);
        return -EOPNOTSUPP;
    }
    r = kzalloc(sizeof(*r), GFP_KERNEL);
    if (!r) return -ENOMEM;
    r->meta = kvcalloc(s->ini_cb.qp->sq.wqe_cnt, sizeof(*r->meta), GFP_KERNEL);
    r->done = bitmap_zalloc(s->ini_cb.qp->sq.wqe_cnt, GFP_KERNEL);
    if (!r->meta || !r->done) {
        kvfree(r->meta); bitmap_free(r->done); kfree(r);
        return -ENOMEM;
    }
    g = &sched->rr_groups[s->srmc_idx / MLX5_SRM_REROUTE_PATHS];
    r->group = g;
    s->rr = r;
    g->paths[p] = s;
    c->state = p < 2 ? MLX5_SRM_ROUTE_ACTIVE : MLX5_SRM_ROUTE_SPARE;
    c->user_db = p < 2;
    c->abi = MLX5_SRM_REROUTE_ABI | MLX5_SRM_ENABLE_CQE_SIMPLIFY << 16;
    c->active[0] = 0;
    c->active[1] = 1;
    if (p >= 2) WRITE_ONCE(s->ctrl_page->resv_idx, MLX5_SRM_REROUTE_FROZEN);
    smp_store_release(&c->generation, 1);
    WRITE_ONCE(s->ctrl_page->flags, READ_ONCE(s->ctrl_page->flags) |
               MLX5_SRM_CTRL_F_REROUTE);
    return 0;
}

static void rr_path_free(struct mlx5_ib_srmc *s)
{
    if (!s->rr) return;
    kvfree(s->rr->meta); bitmap_free(s->rr->done);
    kfree(s->rr); s->rr = NULL;
}

/* A transport error is not a congestion event. Keep transaction mappings
 * alive for later hardware flush CQEs; never put this QP back in the pool.
 * The application receives the original error WC and must tear down/restart.
 * This also serves as fail-closed shutdown after the CQ worker has stopped. */
static void rr_fail_group(struct mlx5_srm_rr_group *g, const char *reason)
{
    unsigned int p;
    if (g->phase == RR_FAILED) return;
    for (p = 0; p < MLX5_SRM_REROUTE_PATHS; p++) {
        struct mlx5_ib_srmc *s = g->paths[p];
        if (!s) continue;
        smp_store_release(&s->ctrl_page->route.user_db, 0);
        atomic64_fetch_or(MLX5_SRM_REROUTE_FROZEN,
                         (atomic64_t *)&s->ctrl_page->resv_idx);
        smp_store_release(&s->ctrl_page->route.state, MLX5_SRM_ROUTE_FAILED);
    }
    rr_log(g, reason);
    g->phase = RR_FAILED;
}

void mlx5_srm_rr_fail(struct mlx5_ib_srmc *s)
{
    if (s->rr) rr_fail_group(s->rr->group, "fatal-cqe");
}

static bool rr_lock(struct mlx5_ib_srmc *s)
{
    return cmpxchg(&s->ctrl_page->db_owner, MLX5_SRM_DB_OWNER_FREE,
        MLX5_SRM_DB_OWNER_KERNEL) == MLX5_SRM_DB_OWNER_FREE;
}
static void rr_unlock(struct mlx5_ib_srmc *s)
{
    smp_store_release(&s->ctrl_page->db_owner, MLX5_SRM_DB_OWNER_FREE);
}

static bool rr_begin(struct mlx5_srm_rr_group *g, unsigned int active)
{
    struct mlx5_srm_route_ctrl *route = &g->paths[0]->ctrl_page->route;
    struct mlx5_ib_srmc *src = g->paths[route->active[active]], *dst = NULL;
    u64 reserved;
    unsigned int p;
    if (!src->publish_pages) return false;
    for (p = 0; p < MLX5_SRM_REROUTE_PATHS; p++) {
        struct mlx5_ib_srmc *s = g->paths[p];
        if (s && s->publish_pages &&
            READ_ONCE(s->ctrl_page->route.state) == MLX5_SRM_ROUTE_SPARE &&
            mlx5_srm_rr_seq(READ_ONCE(s->ctrl_page->resv_idx)) ==
                READ_ONCE(s->ctrl_page->cons_idx)) {
            dst = s; break;
        }
    }
    if (!dst || !rr_lock(src)) return false;
    g->timing_start_cycles = rdtsc_ordered();
    g->timing_prewrite_cycles = 0;
    g->timing_reported = false;
    smp_store_release(&src->ctrl_page->route.user_db, 0);
    /* fetch_or is the linearization point for the producer CAS. */
    reserved = atomic64_fetch_or(MLX5_SRM_REROUTE_FROZEN,
                                (atomic64_t *)&src->ctrl_page->resv_idx);
    g->src = src; g->dst = dst; g->source_slot = active;
    g->db_stop = READ_ONCE(src->ctrl_page->db_tail);
    g->freeze_end = mlx5_srm_rr_seq(reserved);
    g->cursor = g->db_stop;
    g->bytes = g->copied = g->outstanding = 0;
    g->copying_done = false;
    g->phase = RR_READY;
    g->started = g->last_warn = jiffies;
    WRITE_ONCE(src->ctrl_page->route.state, MLX5_SRM_ROUTE_WAIT_READY);
    rr_unlock(src);
    rr_log(g, "freeze");
    return true;
}

/* One maintenance completion is included in the shared CQ credit budget.
 * It never counts as an application completion or contributes payload bytes. */
static bool rr_retire_sq(struct mlx5_srm_rr_group *g)
{
    struct mlx5_ib_srmc *s = g->src;
    struct mlx5_sq_ctrl_page *credit =
        READ_ONCE(sched_group.scheds[0].workers[0].credit_ctrl);
    struct mlx5_ib_qp *qp = s->ini_cb.qp;
    struct mlx5_wqe_ctrl_seg *last;
    s64 issued, completed;
    if (!credit || !rr_lock(s)) return false;
    issued = atomic64_read(&credit->issued_total);
    completed = atomic64_read(&credit->completed_total);
    if (issued - completed >= READ_ONCE(credit->credit_limit) ||
        atomic64_cmpxchg(&credit->issued_total, issued, issued + 1) != issued) {
        rr_unlock(s); return false;
    }
    last = rr_wqe(s, g->freeze_end - 1);
    last->fm_ce_se = MLX5_WQE_CTRL_CQ_UPDATE;
    s->rr->maintenance = true;
    s->rr->maintenance_end = g->freeze_end;
    qp->sq.cur_post = (u32)g->freeze_end;
    qp->sq.head = (u32)g->db_stop;
    qp->bf.offset = READ_ONCE(s->ctrl_page->bf_offset);
    mlx5r_ring_db(qp, mlx5_srm_rr_seq(g->freeze_end - g->db_stop), last);
    WRITE_ONCE(s->ctrl_page->bf_offset, qp->bf.offset);
    smp_store_release(&s->ctrl_page->db_tail, g->freeze_end);
    rr_unlock(s);
    g->phase = RR_NOP_WAIT;
    rr_log(g, "retire-nop-db");
    return true;
}

bool mlx5_srm_rr_maintenance_post(struct mlx5_ib_srmc *s, u16 counter,
                                  u64 *post)
{
    if (!s->rr || !s->rr->maintenance ||
        counter != (u16)(s->rr->maintenance_end - 1)) return false;
    *post = mlx5_srm_rr_seq(s->rr->maintenance_end - 1);
    return true;
}

/* Called once per visit. Runtime disable stops only new detection, never
 * abandons a live transaction. A hole leaves the SAME frozen cursor intact. */
static void rr_step(struct mlx5_srm_rr_group *g)
{
    struct mlx5_ib_srmc *s = g->src, *d = g->dst;
    u32 budget = MLX5_SRM_REROUTE_BUDGET;
    u64 n, tail;
    if (g->phase == RR_IDLE || g->phase == RR_FAILED) return;
    if (g->phase == RR_READY) {
        while (g->cursor != g->freeze_end && budget--) {
            struct mlx5_wqe_ctrl_seg *w;
            u32 op, ds;
            if (!rr_ready(s, g->cursor)) { rr_warn(g, "wait-hole"); return; }
            w = rr_wqe(s, g->cursor);
            op = be32_to_cpu(w->opmod_idx_opcode);
            ds = be32_to_cpu(w->qpn_ds);
            if ((op & 255) != MLX5_OPCODE_RDMA_READ &&
                (op & 255) != MLX5_OPCODE_RDMA_WRITE) {
                rr_warn(g, "invalid-opcode"); return;
            }
            if ((ds & 63) != 4 || (ds >> 8) != s->ini_cb.qp->ibqp.qp_num ||
                (u16)(op >> 8) != (u16)g->cursor) {
                rr_warn(g, "invalid-descriptor"); return;
            }
            g->bytes += rr_bytes(s, g->cursor);
            g->cursor = mlx5_srm_rr_seq(g->cursor + 1);
        }
        if (g->cursor != g->freeze_end) return;
        g->copy = g->bytes >= READ_ONCE(srm_reroute_copy_threshold_bytes) &&
                  g->db_stop != g->freeze_end;
        rr_log(g, g->copy ? "classify-copy" : "classify-drain");
        bitmap_zero(g->dependent, MLX5_SRM_REROUTE_USERS);
        g->cursor = READ_ONCE(s->cq_complete_idx);
        g->phase = g->copy ? RR_SNAPSHOT : RR_RESERVE;
        return;
    }
    if (g->phase == RR_SNAPSHOT) {
        /* Snapshot endpoints per user, not stale inflight counters. CQ may
         * advance during this bounded scan; endpoint gates remain correct. */
        while (g->cursor != g->db_stop && budget--) {
            u16 user = mlx5_srm_publish_usr_rc(
                mlx5_ib_srmc_get_publish_token(s, g->cursor));
            __set_bit(user, g->dependent);
            g->cursor = mlx5_srm_rr_seq(g->cursor + 1);
            g->dep_end[user] = g->cursor;
        }
        if (g->cursor == g->db_stop) g->phase = RR_RESERVE;
        return;
    }
    if (g->phase == RR_RESERVE) {
        struct mlx5_srm_route_ctrl *route = &g->paths[0]->ctrl_page->route;
        n = g->copy ? mlx5_srm_rr_seq(g->freeze_end - g->db_stop) : 0;
        if (n > (u64)d->ini_cb.qp->sq.wqe_cnt * 2 / 3) {
            rr_warn(g, "wait-destination-space"); return;
        }
        if (!rr_lock(d)) return;
        tail = READ_ONCE(d->ctrl_page->cons_idx);
        g->dst_begin = tail;
        g->dst_end = mlx5_srm_rr_seq(tail + n);
        WRITE_ONCE(d->ctrl_page->route.state, MLX5_SRM_ROUTE_TARGET);
        WRITE_ONCE(d->ctrl_page->route.user_db, 0);
        smp_store_release(&d->ctrl_page->resv_idx, g->dst_end);
        rr_unlock(d);
        smp_store_release(&route->active[g->source_slot],
                          d->srmc_idx % MLX5_SRM_REROUTE_PATHS);
        g->last_posted[g->source_slot] = READ_ONCE(d->ctrl_page->route.posted_bytes);
        g->last_completed[g->source_slot] = d->rr->completed_bytes;
        smp_store_release(&route->generation, READ_ONCE(route->generation) + 1);
        /* The generation release publishes the new active path to userspace.
         * From this point producers may reserve and prefill the target SQ,
         * while route.user_db remains closed. */
        g->timing_prewrite_cycles = rdtsc_ordered();
        g->cursor = g->db_stop;
        g->phase = g->copy ? RR_COPY0 : RR_OLD_DRAIN;
        WRITE_ONCE(s->ctrl_page->route.state,
                   g->copy ? MLX5_SRM_ROUTE_COPY : MLX5_SRM_ROUTE_DRAIN);
        rr_log(g, "prefill-open");
        return;
    }
    if (g->phase == RR_COPY0 || g->phase == RR_COPY1) {
        while (g->cursor != g->freeze_end && budget--) {
            u64 old = g->cursor;
            u64 token = mlx5_ib_srmc_get_publish_token(s, old);
            u16 user = mlx5_srm_publish_usr_rc(token);
            bool blocked = test_bit(user, g->dependent);
            g->cursor = mlx5_srm_rr_seq(old + 1);
            if (blocked != (g->phase == RR_COPY1)) continue;
            tail = mlx5_srm_rr_seq(g->dst_begin + g->copied);
            {
                struct mlx5_wqe_ctrl_seg *w = rr_wqe(d, tail);
                struct rr_meta *m = &d->rr->meta[
                    tail & (d->ini_cb.qp->sq.wqe_cnt - 1)];
                memcpy(w, rr_wqe(s, old), 64);
                w->opmod_idx_opcode = cpu_to_be32(
                    ((u16)tail << 8) | (be32_to_cpu(w->opmod_idx_opcode) & 0xff0000ff));
                w->qpn_ds = cpu_to_be32((d->ini_cb.qp->ibqp.qp_num << 8) | 4);
                m->origin = s;
                m->post = old;
#if MLX5_SRM_ENABLE_WQE_TIMING
                {
                    size_t src_off = MLX5_SRM_TIMING_OFFSET(s->publish_depth) +
                        (old & (s->publish_depth - 1)) *
                            sizeof(struct mlx5_srm_wqe_timestamp);
                    size_t dst_off = MLX5_SRM_TIMING_OFFSET(d->publish_depth) +
                        (tail & (d->publish_depth - 1)) *
                            sizeof(struct mlx5_srm_wqe_timestamp);
                    struct mlx5_srm_wqe_timestamp *st =
                        page_address(s->publish_pages[src_off / PAGE_SIZE]) +
                        src_off % PAGE_SIZE;
                    struct mlx5_srm_wqe_timestamp *dt =
                        page_address(d->publish_pages[dst_off / PAGE_SIZE]) +
                        dst_off % PAGE_SIZE;
                    WRITE_ONCE(dt->post_tsc, READ_ONCE(st->post_tsc));
                    smp_store_release(&dt->sequence, tail + 1);
                }
#endif
                rr_token(d, tail, user);
                g->copied++; g->outstanding++;
            }
        }
        if (g->cursor != g->freeze_end) return;
        if (g->phase == RR_COPY0) {
            g->phase = RR_COPY1; g->cursor = g->db_stop; return;
        }
        g->copying_done = true;
        g->phase = RR_COPY_DRAIN;
        rr_log(g, "copy-complete");
        /* The scheduler may now DB an eligible copied/prefilled prefix;
         * per-user old-path dependencies remain enforced by rr_can_db(). */
        rr_report_kernel_db_eligible(g);
        return;
    }
    if (g->phase == RR_COPY_DRAIN || g->phase == RR_OLD_DRAIN) {
        u64 end = g->copy ? g->db_stop : g->freeze_end;
        if (mlx5_srm_rr_delta(READ_ONCE(s->cq_complete_idx), end) < 0) {
            rr_warn(g, "wait-old-cqe"); return;
        }
        if (!rr_lock(d)) return;
        WRITE_ONCE(d->ctrl_page->route.state, MLX5_SRM_ROUTE_ACTIVE);
        smp_store_release(&d->ctrl_page->route.user_db, 1);
        rr_unlock(d);
        WRITE_ONCE(s->ctrl_page->route.state, MLX5_SRM_ROUTE_RETIRED);
        g->phase = RR_RECYCLE;
        g->cursor = g->db_stop;
        rr_log(g, "new-db-open");
        /* Drain mode has no copied prefix. The target becomes DB-eligible
         * only after the complete old source interval has drained. */
        if (!g->copy)
            rr_report_kernel_db_eligible(g);
        return;
    }
    if (g->phase == RR_RECYCLE) {
        if (g->outstanding) return;
        /* Do not expose capacity before hardware has consumed the skipped
         * slots: otherwise prefilling a reused SQ can overwrite its NOPs. */
        if (g->copy) {
            while (g->cursor != g->freeze_end && budget--) {
                struct mlx5_wqe_ctrl_seg *w = rr_wqe(s, g->cursor);
                memset(w, 0, 64);
                w->opmod_idx_opcode = cpu_to_be32(((u16)g->cursor << 8) |
                                                MLX5_OPCODE_NOP);
                w->qpn_ds = cpu_to_be32((s->ini_cb.qp->ibqp.qp_num << 8) | 1);
                g->cursor = mlx5_srm_rr_seq(g->cursor + 1);
            }
            if (g->cursor != g->freeze_end) return;
            rr_retire_sq(g);
            return;
        }
        g->phase = RR_NOP_WAIT;
    }
    if (g->phase == RR_NOP_WAIT) {
        if (s->rr->maintenance) { rr_warn(g, "wait-retire-cqe"); return; }
        s->sched_post_idx = s->cq_complete_idx = g->freeze_end;
        s->ini_cb.qp->sq.cur_post = s->ini_cb.qp->sq.head =
            s->ini_cb.qp->sq.tail = (u32)g->freeze_end;
        WRITE_ONCE(s->ctrl_page->db_tail, g->freeze_end);
        s->rr->logical_cons = g->freeze_end;
        smp_store_release(&s->ctrl_page->cons_idx, g->freeze_end);
        WRITE_ONCE(s->ctrl_page->route.state, MLX5_SRM_ROUTE_SPARE);
        g->last_switch_ns = ktime_get_ns();
        g->bad[0] = g->bad[1] = 0;
        rr_log(g, "recycled");
        g->phase = RR_IDLE; g->src = g->dst = NULL;
    }
}

static void rr_detect(struct mlx5_srm_rr_group *g, u64 now_ns)
{
    struct mlx5_srm_route_ctrl *route;
    u64 posted[2], completed[2], ratio[2];
    u64 cooldown_ns;
    unsigned int p, poor;
    int forced;
    for (p = 0; p < MLX5_SRM_REROUTE_PATHS; p++)
        if (!g->paths[p]) return;
    if (g->phase != RR_IDLE) return;
    route = &g->paths[0]->ctrl_page->route;
    forced = READ_ONCE(srm_reroute_test_source);
    for (p = 0; p < 2; p++)
        if (forced == g->paths[route->active[p]]->srmc_idx &&
            rr_begin(g, p)) {
            cmpxchg(&srm_reroute_test_source, forced, -1);
            return;
        }
    if (now_ns < g->next_check_ns) return;
    g->next_check_ns = now_ns +
        (u64)max(1U, READ_ONCE(srm_reroute_interval_us)) * NSEC_PER_USEC;
    for (p = 0; p < 2; p++) {
        struct mlx5_srm_route_ctrl *c = &g->paths[route->active[p]]->ctrl_page->route;
        u64 a = smp_load_acquire(&c->posted_bytes);
        u64 b = g->paths[route->active[p]]->rr->completed_bytes;
        posted[p] = a - g->last_posted[p];
        completed[p] = b - g->last_completed[p];
        g->last_posted[p] = a; g->last_completed[p] = b;
        ratio[p] = !posted[p] || completed[p] >= posted[p] ? 1000 :
            div64_u64(completed[p] * 1000, posted[p]);
    }
    rr_detect_log(g, route, posted, completed, ratio, now_ns);
    cooldown_ns = (u64)READ_ONCE(srm_reroute_cooldown_us) *
                  NSEC_PER_USEC;
    if (!READ_ONCE(srm_reroute_enable) || !posted[0] || !posted[1] ||
        (g->last_switch_ns &&
         now_ns - g->last_switch_ns < cooldown_ns)) {
        g->bad[0] = g->bad[1] = 0; return;
    }
    poor = ratio[0] < ratio[1] ? 0 : 1;
    g->bad[poor ^ 1] = 0;
    if (ratio[poor ^ 1] - ratio[poor] < READ_ONCE(srm_reroute_ratio_gap)) {
        g->bad[poor] = 0; return;
    }
    if (++g->bad[poor] >= max(1U, READ_ONCE(srm_reroute_bad_windows)))
        rr_begin(g, poor);
}

/* Two virtual active slots per group. Drain-only source substitutes for its
 * gated target; the source cannot disappear merely because routing changed. */
static u32 rr_schedule(struct mlx5_ib_sched *sched, u32 virtual_slot,
                       u64 now_ns)
{
    struct mlx5_srm_rr_group *g = &sched->rr_groups[virtual_slot / 2];
    u32 slot = virtual_slot % 2;
    struct mlx5_srm_route_ctrl *route;
    if (!g->paths[0]) return U32_MAX;
    rr_step(g);
    rr_detect(g, now_ns);
    if (g->phase == RR_OLD_DRAIN && slot == g->source_slot)
        return g->src->srmc_idx;
    route = &g->paths[0]->ctrl_page->route;
    return g->id * MLX5_SRM_REROUTE_PATHS + READ_ONCE(route->active[slot]);
}

bool mlx5_srm_rr_can_db(struct mlx5_ib_srmc *s, u64 post)
{
    struct mlx5_srm_rr_group *g;
    u16 user;
    if (!s->rr) return false;
    g = s->rr->group;
    switch (READ_ONCE(s->ctrl_page->route.state)) {
    case MLX5_SRM_ROUTE_ACTIVE: return true;
    case MLX5_SRM_ROUTE_DRAIN:
        return mlx5_srm_rr_delta(g->freeze_end, post) > 0;
    case MLX5_SRM_ROUTE_TARGET:
        if (!g->copying_done ||
            mlx5_srm_rr_delta(g->dst_end, post) <= 0 ||
            !rr_ready(s, post)) return false;
        user = mlx5_srm_publish_usr_rc(mlx5_ib_srmc_get_publish_token(s, post));
        return !test_bit(user, g->dependent) ||
            mlx5_srm_rr_delta(READ_ONCE(g->src->cq_complete_idx),
                              g->dep_end[user]) >= 0;
    default: return false;
    }
}

static void rr_publish_cons(struct mlx5_ib_srmc *s)
{
#if MLX5_SRM_ENABLE_CQE_SIMPLIFY && MLX5_SRM_CQE_PUBLISH_BATCH <= 1 && !MLX5_SRM_ENABLE_PRIVATE_CQ
    smp_store_release(&s->ctrl_page->cons_idx, s->rr->logical_cons);
#else
    struct mlx5_ib_sched *sched = &sched_group.scheds[0];
    if (!s->rr->publish_pending) {
        s->rr->publish_pending = true;
        s->rr->publish_next = sched->rr_publish_head;
        sched->rr_publish_head = s;
    }
#endif
}

/* An exclusive CQ knows its KQP. In RR_IDLE all requests complete on their
 * original SQ, so one physical cursor also gives the logical cursor. The
 * caller checked every CQE counter; only payload accounting remains here.
 * Migration and maintenance use rr_complete() one CQE at a time instead. */
#if MLX5_SRM_ENABLE_PRIVATE_CQ && MLX5_SRM_ENABLE_CQE_SIMPLIFY
bool mlx5_srm_rr_idle(const struct mlx5_ib_srmc *s)
{
    return s->rr && READ_ONCE(s->rr->group->phase) == RR_IDLE;
}

void mlx5_srm_rr_complete_idle_batch(struct mlx5_ib_srmc *s,
                                     u64 first, u32 count)
{
    u32 i;

    if (READ_ONCE(srm_reroute_enable)) {
        u64 bytes = 0;

        for (i = 0; i < count; i++)
            bytes += rr_bytes(s, mlx5_srm_rr_seq(first + i));
        s->rr->completed_bytes += bytes;
    }
    s->rr->logical_cons = mlx5_srm_rr_seq(first + count);
    rr_publish_cons(s);
}
#endif

void mlx5_srm_rr_flush_native(struct mlx5_ib_sched *sched)
{
    struct mlx5_ib_srmc *s;
    while ((s = sched->rr_publish_head)) {
        sched->rr_publish_head = s->rr->publish_next;
        s->rr->publish_pending = false;
        s->rr->publish_next = NULL;
        smp_store_release(&s->ctrl_page->cons_idx, s->rr->logical_cons);
    }
}

/* Resolve before publishing a user completion. Credit counts always remain
 * those of the original physical CQ poll, not the remapped logical index. */
int mlx5_srm_rr_complete(struct mlx5_ib_srmc **physical, u64 *post,
                         u32 status, u32 vendor, u64 tsc)
{
    struct mlx5_ib_srmc *d = *physical, *s = d;
    struct mlx5_srm_rr_group *g = d->rr->group;
    struct rr_meta *m;
    bool migrated;
    u64 origin = *post;

    /* RR_IDLE is reached only after all imported completions, source holes
     * and SQ-retirement NOPs have drained. ACTIVE alone is NOT sufficient:
     * the destination can be ACTIVE while imported requests remain in flight.
     * Keep errors on the slow path, even when automatic detection is off. */
    if (likely(!status && g->phase == RR_IDLE)) {
        d->rr->completed_bytes += rr_bytes(d, *post);
        d->rr->logical_cons = mlx5_srm_rr_seq(*post + 1);
#if MLX5_SRM_ENABLE_WQE_TIMING
        mlx5_srm_timing_publish_kernel_cqe(d, *post, tsc);
#endif
        rr_publish_cons(d);
        return 0;
    }

    if (d->rr->maintenance &&
        mlx5_srm_rr_seq(*post + 1) == d->rr->maintenance_end) {
        d->rr->maintenance = false;
        if (status) mlx5_srm_rr_fail(d);
        return 2;
    }
    d->rr->completed_bytes += rr_bytes(d, *post);
    m = &d->rr->meta[*post & (d->ini_cb.qp->sq.wqe_cnt - 1)];
    migrated = m->origin != NULL;
    if (migrated) {
        s = m->origin; origin = m->post;
        m->origin = NULL;
        __set_bit(origin & (s->ini_cb.qp->sq.wqe_cnt - 1), s->rr->done);
        g->outstanding--;
        d->rr->logical_cons = mlx5_srm_rr_seq(*post + 1);
        rr_publish_cons(d);
    } else {
        s->rr->logical_cons = mlx5_srm_rr_seq(origin + 1);
    }
    while (test_bit(s->rr->logical_cons &
                    (s->ini_cb.qp->sq.wqe_cnt - 1), s->rr->done)) {
        __clear_bit(s->rr->logical_cons &
                    (s->ini_cb.qp->sq.wqe_cnt - 1), s->rr->done);
        s->rr->logical_cons = mlx5_srm_rr_seq(s->rr->logical_cons + 1);
    }
#if MLX5_SRM_ENABLE_WQE_TIMING
    mlx5_srm_timing_publish_kernel_cqe(s, origin, tsc);
#endif
    if (status) {
        WRITE_ONCE(s->ctrl_page->completion_error_idx, mlx5_srm_rr_seq(origin + 1));
        WRITE_ONCE(s->ctrl_page->completion_error_status, status);
        WRITE_ONCE(s->ctrl_page->completion_error_vendor, vendor);
    }
#if MLX5_SRM_ENABLE_CQE_SIMPLIFY
    if (migrated || status) {
        u16 user = mlx5_srm_publish_usr_rc(
            mlx5_ib_srmc_get_publish_token(s, origin));
        struct mlx5_srm_migration_completion *r = rr_record(s, user);
        WRITE_ONCE(r->origin_slot, s->srmc_idx);
        WRITE_ONCE(r->status, status); WRITE_ONCE(r->vendor, vendor);
        WRITE_ONCE(r->kernel_tsc, tsc); WRITE_ONCE(r->reserved, 1);
        smp_store_release(&r->sequence, mlx5_srm_rr_seq(origin + 1));
    }
#endif
    rr_publish_cons(s);
    if (status) mlx5_srm_rr_fail(d);
    *physical = s; *post = origin;
    return migrated;
}
#endif
