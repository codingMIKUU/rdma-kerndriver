
#ifndef _MLX5_IB_SCHEDULER_H
#define _MLX5_IB_SCHEDULER_H

#include <linux/mutex.h>
#include <linux/types.h>
#include <rdma/rdma_cm.h>
#define SQ_DEPTH 8192
static int debug = 0; 
#define NUM_SRMC 8192
//对于接收端，NUM_SRMC等于num_kqps*2才行（因为只有一个调度器，发送端两个调度器全发往它了）
#define NUM_SQB 1024


#define DEBUG_LOG \
    if (debug) \
    printk

// 5. 缓存行对齐（优化缓存友好性）
#define CACHELINE_ALIGNED __cacheline_aligned

struct mlx5_ib_cqbuf{
    void* buf;
    struct page** pages;
    size_t cq_size;
    int cqe_sz;
    int cqn;
    int cur_put;
    int op_own;
    struct mutex lock;
    struct mlx5_ib_cqbuf* next;//not loop
}CACHELINE_ALIGNED;
struct mlx5_ib_sqbuf {
    void* buf;
    struct page** pages;
    size_t sq_size;
    uint32_t wqe_cnt;
    int qpn;
    u32 uidx;
    uint32_t cur_post;
    struct mlx5_ib_cqbuf* cqb;
    struct mlx5_ib_sqbuf* next;//not loop


    int idx;//该sqb在sched_group中的索引，仅用于debug
}CACHELINE_ALIGNED;
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
enum MESSAGE_SIZE{
    MESSAGE_SIZE_SMALL = 0,
    MESSAGE_SIZE_LARGE = 1,
};
struct srm_cb{
    int refcnt;
    struct mlx5_ib_qp *qp;
    struct ib_cq *cq;
    
    int page_list_len;
    char* dma_buf;
    DEFINE_DMA_UNMAP_ADDR(dma_mapping);
    struct ib_mr *mr;
    char* buf;
    int buf_sz;

    struct rdma_cm_id *cm_id;
    enum test_state state;
    wait_queue_head_t sem;
    int txdepth;
    struct ib_pd *pd;
    struct ib_xrcd *xrcd;
    int server;
    char *addr_str;
    uint16_t port;			 /* dst port in NBO */
	u8 addr[16];			 /* dst addr in NBO */

};
struct buf_info{
    char* addr;
    int buf_sz;
    u32 rkey;
};
struct mlx5_wqe_info{
    u32 qpn;
    struct mlx5_ib_sqbuf* sqb;
    u16 wqe_counter;
    size_t pending_bytes;
    size_t byte_cnt;
    u8 to_user;
    u8 valid;
};
struct mlx5_ib_srmc {
   struct srm_cb ini_cb;
   struct srm_cb tgt_cb;
   union ib_gid dgid;
   int sig_cnt;
   uint32_t cur_cqe;
   struct mlx5_wqe_info wqe_infos[SQ_DEPTH];
   size_t pending_bytes;//total pending bytes
   size_t cul_pending_bytes;//next cqe's pending bytes
   int idx;// 该srmc在表中的索引
   int srmc_idx; //该srmc在创建顺序中排第几个(用于分配cq)
}CACHELINE_ALIGNED;
struct mlx5_ib_sched{
    struct task_struct* task;
    struct mutex srmc_lock;
    struct mlx5_ib_srmc* srmc_small_tb[NUM_SRMC];//for small flows
    struct mlx5_ib_srmc* srmc_large_tb[NUM_SRMC];//for large flows
    size_t srmc_cnt[2];
    int id;
};
struct mlx5_ib_sched_id{
    struct mlx5_ib_sched* sched;
    int id;
};
struct mlx5_ib_sched_group{
    struct mutex sq_lock;

    uint32_t sqb_cnt;
    uint32_t cqb_cnt;
    struct mlx5_ib_sqbuf* sqb_arr[NUM_SQB];
    struct mutex cq_lock;
    struct mlx5_ib_cqbuf* cqb_arr[NUM_SQB];
    
    int num_sched;
    struct mlx5_ib_sched *scheds;
};
struct mlx5_ib_server{
    int cm_id;
    struct task_struct *task;
    struct srm_cb server_cb;
};
enum srmc_create_flag{

    SRMC_CREATE_FLAG_INIT_QP = 1,
    SRMC_CREATE_FLAG_TGT_QP = 2,
};

int mlx5_ib_map_ubuf(struct mlx5_ib_sched_group* sched_group,unsigned long virt_addr,size_t size,int qpn,int cqn,u32 uidx);
int mlx5_ib_map_cq_ubuf(struct mlx5_ib_sched_group* sched_group,unsigned long virt_addr,size_t size,int cqn);
int scheduler_polling(void* sched_data);
int mlx5_ib_create_srmc(struct mlx5_ib_sched* sched,struct mlx5_ib_qp *init_qp,struct mlx5_ib_qp *tgt_qp,union ib_gid *dgid);
int mlx5_ib_sched_init(struct mlx5_ib_sched_group* sched_group,int num);
void mlx5_ib_sched_exit(struct mlx5_ib_sched_group* sched_group);
//flags == 1: check ini, flags == 2: check tgt
//If no exists, return -1 and create the srmc, if init qp then create kernel qp;else return 1 and add the refcnt of that qp
int is_xrc_exists(struct mlx5_ib_sched* sched,struct ib_pd *pd,union ib_gid *dgid,int flags,int qpn);

int mlx5_ib_unmap_ubuf(struct mlx5_ib_sched_group* sched_group,int qpn);
int mlx5_ib_destroy_srmc(struct mlx5_ib_sched* sched,int ah_id);
int mlx5_ib_create_srmc_qp(struct mlx5_ib_sched* sched,struct mlx5_ib_srmc *srmc,struct ib_pd *pd,int flags,int qpn);
int mlx5_sched_run_server(struct srm_cb *cb);
int create_srmc_qp_cm(struct mlx5_ib_srmc *srmc,struct ib_pd *pd,union ib_gid *dgid,int flags,int id);
void ib_sched_free_buf(struct srm_cb *cb);
int sched_hash_ip(char addr[4],int n);
int srm_accept(struct srm_cb *cb);
void mlx5_ib_sched_exit(struct mlx5_ib_sched_group* sched_group);
int mlx5_ib_server_init(struct mlx5_ib_server* server);
int polling_cqe(void *data);
int mlx5_ib_register_external_table(void *table, size_t size, struct page **pages, 
    void *level_table, size_t level_size, struct page **level_pages,
    void *idx_table, size_t idx_size, struct page **idx_pages);


#endif /* _MLX5_IB_SCHEDULER_H */