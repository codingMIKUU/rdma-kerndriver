
#ifndef _MLX5_IB_SCHEDULER_H
#define _MLX5_IB_SCHEDULER_H

#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <rdma/rdma_cm.h>

#define SQ_DEPTH 35000
static int debug = 0;
#define NUM_SRMC 1024
// 对于接收端，NUM_SRMC等于num_kqps*2才行（因为只有一个调度器，发送端两个调度器全发往它了）
#define NUM_SQB 35000
#define NUM_LEVEL 2
#define MLX5_SRM_ENABLE_LARGE_KERNEL_QP 0
#define MLX5_SRM_KERNEL_QP_LEVELS \
    (MLX5_SRM_ENABLE_LARGE_KERNEL_QP ? NUM_LEVEL : 1)

#define IP_ADDR "192.168.1.5"
#define PORT_NUM 12345
#define SRMC_POLLING_CNT 8192
#define WQES_ARR_SZ 31
#define NUM_SCHED 1

// 2. 位运算替代取模（需确保CQ_NUM是2的幂，如16、32）
#define CQ_NUM_POWER 0 // 示例：CQ_NUM=2^4=16
#define CQ_NUM (1 << CQ_NUM_POWER)
#define CQ_MOD(srmc_idx) ((srmc_idx) & (CQ_NUM - 1)) // 位运算替代取模

// 3. 提前计算索引宏（减少循环内重复计算）
#define LEVEL_TABLE_IDX(level, id) ((level) + (NUM_LEVEL) * (id))       // level_table索引
#define NUM_THREAD_QPS_PER_SCHED(num_sched) ((NUM_LEVEL) * (num_sched)) // 每个调度器的线程QP数
#define CALC_N(k, num_thread_qps_per_sched, level, num_user_threads, id_per_thread_qp_nums) \
    ((k) / (num_thread_qps_per_sched) + (level) * (num_user_threads) + (id_per_thread_qp_nums))

#define MAX_USER_THREADS_NUM 17
#define MAX_SRM_APPS_NUM 4
#define MAX_USER_XRC_QP_PER_SRM 2048
#define MAX_USER_XRC_QP_PER_SRM 2048


static const size_t MESSAGE_SIZE_THRESHOLD = 1024 * 10;
// const size_t MESSAGE_SIZE_THRESHOLD = 1e9;
#define MLX5_SRM_SCHED_SIZE_LIMIT (4 * 1024)
static const size_t SCHED_SIZE_LIMIT = MLX5_SRM_SCHED_SIZE_LIMIT;
#define MLX5_SRM_ENABLE_SCHED_SIZE_LIMIT 0
#define MLX5_SRM_SCHED_SIZE_LIMIT_ACTIVE (MLX5_SRM_ENABLE_SCHED_SIZE_LIMIT)

#define MLX5_SRM_LARGE_DB_LIMIT 1000
static const u32 LARGE_DB_LIMIT = MLX5_SRM_LARGE_DB_LIMIT;
#define MLX5_SRM_ENABLE_LARGE_DB_LIMIT 0
#define MLX5_SRM_LARGE_DB_LIMIT_ACTIVE \
    (MLX5_SRM_ENABLE_LARGE_KERNEL_QP && \
     MLX5_SRM_ENABLE_LARGE_DB_LIMIT && (MLX5_SRM_LARGE_DB_LIMIT > 0))

#define MLX5_SRM_ENABLE_DB_BATCH_LOG 1
#define MLX5_SRM_DB_BATCH_LOG_INTERVAL (1ULL << 16)

/*
 * Experimental shared-SQ ready-counter fast path.  The matching switch in
 * rdma-core/providers/mlx5/mlx5.h must also be enabled.  Keep this disabled by
 * default: ready_idx is a multi-producer atomic hot spot when enabled.
 */
#define MLX5_SRM_ENABLE_READY_FASTPATH 0

static u64 LIMIT_BATCHING = 20000;
#define DEBUG_LOG \
    if (debug)    \
    printk

// 5. 缓存行对齐（优化缓存友好性）
#define CACHELINE_ALIGNED __cacheline_aligned
#define CACHELINE_ALIGNED_USER __attribute__((__aligned__(64)))


struct srm_qp_entry{
	uint32_t qp_idx;
	uint32_t valid;
    uint64_t ctrl;
    uint64_t bytes;
    uint64_t cycles;
}CACHELINE_ALIGNED_USER;

struct mlx5_sq_ctrl_page {
    __u64 resv_idx;
    __u64 ready_idx;
    __u8 resv_pad[48];
    __u64 cons_idx;
    __u8 cons_pad[56];
    __u64 db_tail;
    __u32 db_owner;
    __u32 flags;
    __u32 bf_offset;
    __u32 direct_db_batch;
    __u32 direct_stats_attempts;
    __u32 direct_stats_not_head;
    __u32 direct_stats_owner_busy;
    __u32 direct_stats_owner_acquired;
    __u32 direct_stats_no_pending;
    __u32 direct_stats_scan_calls;
    __u32 direct_stats_scan_ready_wqes;
    __u32 direct_stats_no_ready;
    __u32 direct_stats_credit_stalls;
    __u32 direct_stats_partial_credit;
    atomic64_t issued_total;
    atomic64_t completed_total;
    __u64 credit_limit;
    __u32 direct_stats_db_calls;
    __u32 direct_stats_db_wqes;
    __u32 direct_stats_db_max;
    __u64 completion_error_idx;
    __u32 completion_error_status;
    __u32 completion_error_vendor;
    /* Must exactly match rdma-core/providers/mlx5/mlx5.h. */
    __u8 credit_pad[8];
	/* Dedicated per-worker latest-hot mailbox cacheline. */
	__u64 latest_hot_hint;
	__u8 hot_hint_pad[56];
	/* Power-of-two stride: a slot must never cross discontiguous pool pages. */
	__u8 slot_pad[192];
} CACHELINE_ALIGNED_USER;
static_assert(sizeof(struct mlx5_sq_ctrl_page) == 512);

#define MLX5_SRM_DB_OWNER_FREE   0U
#define MLX5_SRM_DB_OWNER_USER   1U
#define MLX5_SRM_DB_OWNER_KERNEL 2U
#define MLX5_SRM_CTRL_F_DIRECT_DB_STATS (1U << 0)
#define MLX5_SRM_DIRECT_DB_MAX_BATCH 32U

#define MLX5_SRM_PUBLISH_USR_BITS 16
#define MLX5_SRM_PUBLISH_USR_MASK ((1ULL << MLX5_SRM_PUBLISH_USR_BITS) - 1)
#define MLX5_SRM_PUBLISH_SEQ_MASK ((1ULL << 48) - 1)

/* Logical scheduler ABI remains one; each user CQ has one worker owner. */
static_assert(NUM_SCHED == 1);

static inline u64 mlx5_srm_publish_token(u64 seq, u16 usr_rc_cnt)
{
    return ((seq & MLX5_SRM_PUBLISH_SEQ_MASK) <<
            MLX5_SRM_PUBLISH_USR_BITS) | usr_rc_cnt;
}

static inline u64 mlx5_srm_publish_seq(u64 token)
{
    return token >> MLX5_SRM_PUBLISH_USR_BITS;
}

static inline u16 mlx5_srm_publish_usr_rc(u64 token)
{
    return token & MLX5_SRM_PUBLISH_USR_MASK;
}

#define MLX5_SRM_WRID_KQP_SHIFT 48
#define MLX5_SRM_WRID_POST_MASK MLX5_SRM_PUBLISH_SEQ_MASK

static inline u64 mlx5_srm_make_wrid(u32 kqp_idx, u64 post_idx)
{
    return ((u64)kqp_idx << MLX5_SRM_WRID_KQP_SHIFT) |
           (post_idx & MLX5_SRM_WRID_POST_MASK);
}

static inline u32 mlx5_srm_wrid_kqp(u64 wrid)
{
    return wrid >> MLX5_SRM_WRID_KQP_SHIFT;
}

static inline u64 mlx5_srm_wrid_post(u64 wrid)
{
    return wrid & MLX5_SRM_WRID_POST_MASK;
}

/*
 * Sequence cursors are allowed to wrap.  All live SRM windows are many
 * orders of magnitude smaller than half of the u64 sequence space, so a
 * signed modular delta gives an unambiguous before/after relation.
 */
static inline s64 mlx5_srm_seq_delta(u64 lhs, u64 rhs)
{
    return (s64)(lhs - rhs);
}

static inline bool mlx5_srm_seq_after(u64 lhs, u64 rhs)
{
    return mlx5_srm_seq_delta(lhs, rhs) > 0;
}

static inline u64 mlx5_srm_extend_post48(u64 reference, u64 post48)
{
    const u64 period = MLX5_SRM_WRID_POST_MASK + 1;
    const u64 half = period >> 1;
    u64 delta = ((post48 & MLX5_SRM_WRID_POST_MASK) -
                 (reference & MLX5_SRM_WRID_POST_MASK)) &
                MLX5_SRM_WRID_POST_MASK;

    if (delta >= half)
        return reference - (period - delta);

    return reference + delta;
}

struct mlx5_ib_cqbuf
{
    void *buf;
    struct page **pages;
    size_t cq_size;
    int cqe_sz;
    int cqn;
    int cur_put;
    int op_own;
    u16 owner_worker;
    u64 retire_epoch;
    struct mlx5_ib_cqbuf *next; // not loop
} CACHELINE_ALIGNED;
struct mlx5_ib_sqbuf
{
    struct srm_qp_entry *buf;
    struct page **pages;
    size_t sq_size;
    uint32_t wqe_cnt;
    int qpn;
    u32 uidx;
    uint32_t cur_post;
    struct mlx5_sq_ctrl_page *ctrl;
    struct mlx5_ib_cqbuf *cqb;
    struct mlx5_ib_sqbuf *next; // not loop

    int idx; // 该sqb在sched_group中的索引，仅用于debug
} CACHELINE_ALIGNED;
enum test_state
{
    IDLE = 1,
    CONNECT_REQUEST,
    ADDR_RESOLVED,
    ROUTE_RESOLVED,
    CONNECTED,
    RDMA_READ_ADV,
    RDMA_READ_COMPLETE,
    RDMA_WRITE_ADV,
    RDMA_WRITE_COMPLETE,
    ERROR
};
enum MESSAGE_SIZE
{
    MESSAGE_SIZE_SMALL = 0,
    MESSAGE_SIZE_LARGE = 1,
};
struct srm_cb
{
    int refcnt;
    struct mlx5_ib_qp *qp;
    struct ib_cq *cq;

    int page_list_len;
    char *dma_buf;
    DEFINE_DMA_UNMAP_ADDR(dma_mapping);
    struct ib_mr *mr;
    char *buf;
    int buf_sz;

    struct rdma_cm_id *cm_id;
    enum test_state state;
    int cm_error;
    wait_queue_head_t sem;
    bool sem_initialized;
    int txdepth;
    struct ib_pd *pd;
    struct ib_xrcd *xrcd;
    int server;
    char *addr_str;
    uint16_t port; /* dst port in NBO */
    u8 addr[16];   /* dst addr in NBO */
};
struct buf_info
{
    char *addr;
    int buf_sz;
    u32 rkey;
};
struct mlx5_wqe_info
{
    struct mlx5_ib_cqbuf *cqb;
    struct mlx5_sq_ctrl_page *ctrl_page;
    u64 wqe_counter;
    u32 uidx;
    u16 usr_rc_cnt;
    u8 valid;
    u8 flags;
};
struct mlx5_ib_srmc
{
    struct srm_cb ini_cb;
    struct srm_cb tgt_cb;
    union ib_gid dgid;
    int sig_cnt;
    uint32_t cur_cqe;
    u64 sched_post_idx;
    u64 cq_complete_idx;
    struct mlx5_sq_ctrl_page *ctrl_page;
    struct mlx5_wqe_info *wqe_infos;
    int idx;                  // 该srmc在表中的索引
    int srmc_idx;             // 该srmc在创建顺序中排第几个(用于分配cq)
    u16 owner_worker;
    struct page **publish_pages;
    u32 publish_npages;
    u32 publish_depth;
    unsigned long last_db_jiffies;
    unsigned long last_cqe_jiffies;
    unsigned long last_stall_warn_jiffies;
    unsigned long publish_gap_jiffies;
    u64 publish_gap_slot;
} CACHELINE_ALIGNED;
struct mlx5_ib_sched;
struct mlx5_ib_sched_worker
{
    struct mlx5_ib_sched *sched;
    struct task_struct *task;
    struct ib_cq *shared_cq[CQ_NUM];
    u32 worker_id;
    u32 cpu_id;
    u32 kqp_begin;
    u32 kqp_end;
    u64 limit_batch;
    struct mlx5_sq_ctrl_page *credit_ctrl;
    u64 quiescent_epoch;
} CACHELINE_ALIGNED;
struct mlx5_ib_sched
{
    struct mlx5_ib_sched_worker *workers;
    u32 worker_count;
    struct mutex srmc_lock;
    struct mlx5_ib_srmc *srmc_tb[NUM_SRMC]; // single per-gid SRMC
    struct mlx5_ib_srmc *srmc_by_idx[NUM_SRMC]; // direct lookup by ctrl slot / srmc_idx
    size_t srmc_cnt;
    size_t ready_srmc_cnt;
    int init_error;
    wait_queue_head_t init_wait;
    int id;
};

struct mlx5_ib_usr_rc_route {
    struct mlx5_ib_cqbuf *cqb;
    u32 uidx;
};
struct mlx5_ib_sched_id
{
    struct mlx5_ib_sched *sched;
    int id;
    u32 worker_id;
};

struct xrc_table_entry {
  uint64_t ctrl;
  uint64_t tot_bytes;
  uint64_t tot_recv_cqes;

  uint64_t cur_Gbps;
  uint64_t cur_lat_us;
  uint64_t update_cnt;
} CACHELINE_ALIGNED_USER; // 对齐到多少字节？

struct aligned_u32 {
    uint32_t val;
} CACHELINE_ALIGNED_USER;

struct db_rc{
    uint64_t vaddr;
    struct page *page;
    uint32_t *kaddr;
};

struct xrc_bf_entry{
    u64 bf_addr;
    u32 bf_size;
    u32 bf_offset;
    u64 uar_page_vaddr;
    //struct db_rc db;
};

struct mlx5_ib_sched_group
{
    struct mutex sq_lock;
	/* Serializes Hollow RC QP owner attach/detach against SRMC teardown. */
	struct mutex owner_lock;
	struct list_head owner_qps;
	bool owner_stopping;

    uint32_t sqb_cnt;
    uint32_t cqb_cnt;
    uint32_t xrc_bf_cnt;
	struct mlx5_ib_sqbuf *sqb_arr[NUM_SQB];
	struct mutex cq_lock;
	struct mlx5_ib_cqbuf *cqb_arr[NUM_SQB];
	struct mlx5_ib_cqbuf *retired_cqbs;
	struct delayed_work cqb_reclaim_work;
	atomic64_t route_epoch;
	struct mlx5_ib_usr_rc_route usr_rc_routes[NUM_SQB];
	struct xrc_bf_entry *xrc_bf_arr[MAX_USER_THREADS_NUM*NUM_SCHED*NUM_LEVEL*MAX_USER_XRC_QP_PER_SRM];

    int num_sched;
    struct mlx5_ib_sched *scheds;
};
struct mlx5_ib_server
{
    int cm_id;
    struct task_struct *task;
    struct srm_cb server_cb;
    atomic_t conn_tasks;
    wait_queue_head_t conn_wait;
    /* Every accepted child CM ID must remain reachable until teardown. */
    struct mutex conn_lock;
    struct list_head conn_list;
    bool stopping;
};
enum srmc_create_flag
{

    SRMC_CREATE_FLAG_INIT_QP = 1,
    SRMC_CREATE_FLAG_TGT_QP = 2,
};



int mlx5_ib_map_ubuf(struct mlx5_ib_sched_group *sched_group, unsigned long virt_addr, size_t size, int qpn, int cqn, u32 uidx);
int mlx5_ib_map_cq_ubuf(struct mlx5_ib_sched_group *sched_group, unsigned long virt_addr, size_t size, int cqn);
int mlx5_ib_unmap_cq_ubuf(struct mlx5_ib_sched_group *sched_group, int cqn);
int mlx5_ib_bind_usr_rc_cq(struct mlx5_ib_sched_group *sched_group,
			   u32 usr_rc_cnt, int cqn, u32 uidx);
void mlx5_ib_unbind_usr_rc_cq(struct mlx5_ib_sched_group *sched_group,
			      u32 usr_rc_cnt);
struct mlx5_ib_sqbuf *mlx5_ib_find_sqbuf_by_qpn(struct mlx5_ib_sched_group *sched_group, int qpn);
int scheduler_polling(void *sched_data);
int mlx5_ib_create_srmc(struct mlx5_ib_sched *sched, struct mlx5_ib_qp *init_qp, struct mlx5_ib_qp *tgt_qp, union ib_gid *dgid);
int mlx5_ib_sched_init(struct mlx5_ib_sched_group *sched_group, int num);
void mlx5_ib_sched_stop(struct mlx5_ib_sched_group *sched_group);
void mlx5_ib_sched_exit(struct mlx5_ib_sched_group *sched_group);
// flags == 1: check ini, flags == 2: check tgt
// If no exists, return -1 and create the srmc, if init qp then create kernel qp;else return 1 and add the refcnt of that qp
int is_xrc_exists(struct mlx5_ib_sched *sched, struct ib_pd *pd, union ib_gid *dgid, int flags, int qpn, u32 sq_depth);

int mlx5_ib_unmap_ubuf(struct mlx5_ib_sched_group *sched_group, int qpn);
int mlx5_ib_destroy_srmc(struct mlx5_ib_sched *sched, int ah_id);
int mlx5_ib_create_srmc_qp(struct mlx5_ib_sched *sched, struct mlx5_ib_srmc *srmc, struct ib_pd *pd, int flags, int qpn);
int mlx5_sched_run_server(void *data);
int create_srmc_qp_cm(struct mlx5_ib_srmc *srmc, struct ib_pd *pd, union ib_gid *dgid, int flags, int id, u32 sq_depth);
void ib_sched_free_buf(struct srm_cb *cb);
int sched_hash_ip(char addr[4], int n);
int srm_accept(struct srm_cb *cb);
int mlx5_ib_server_init(struct mlx5_ib_server *server);
void mlx5_ib_server_exit(struct mlx5_ib_server *server,
                         struct mlx5_ib_sched_group *sched_group);
int polling_cqe(void *data);
int mlx5_ib_register_external_table(void *table, size_t size, struct page **pages, void *level_table, size_t level_size, struct page **level_pages,
                                           void *xrc_table, size_t xrc_size, struct page **xrc_pages, int xrc_qp_num_per_srm);
int srm_map_bf(struct mlx5_ib_sched_group *sched_group,struct mlx5_ib_create_qp *ucmd,struct mlx5_ib_dev *dev);
#endif /* _MLX5_IB_SCHEDULER_H */
