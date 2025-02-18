
// #ifndef _MLX5_IB_SCHEDULER_H
// #define _MLX5_IB_SCHEDULER_H

// #include <linux/mutex.h>
// #include <linux/types.h>
// struct mlx5_ib_sqbuf {
//     void* buf;
//     struct page** pages;
//     size_t sq_size;//
//     int qpn;
//     int cqn;
//     int cur_post;
//     struct mlx5_ib_sqbuf* next;//not loop
// };
// struct mlx5_ib_cqbuf{
//     void* buf;
//     struct page** pages;
//     size_t cq_size;
//     int cqe_sz;
//     int cqn;
//     int cur_put;
//     struct mlx5_ib_cqbuf* next;//not loop
// };
// struct mlx5_ib_srmc{
//     int ini_refcnt;
//     int tgt_refcnt;
//     union ib_gid	dgid;
//     int ah_id;
//     struct mlx5_ib_qp *init_qp;
//     struct mlx5_ib_cq *init_cq;//TODO:get init_cq
//     struct ib_mr *ini_mr;
//     int sig_cnt;
//     struct mlx5_ib_qp *tgt_qp;
//     struct ib_mr *tgt_mr;
//     struct mlx5_ib_srmc* next;//not loop
// };
// struct mlx5_ib_sched{
//     struct mutex sq_lock;
//     struct mlx5_ib_sqbuf* sq_head;
//     struct mutex cq_lock;
//     struct mlx5_ib_cqbuf* cq_head;
//     struct task_struct* task;
//     struct mutex srmc_lock;
//     struct mlx5_ib_srmc* srmc_head;
// };
// enum srmc_create_flag{
//     SRMC_CREATE_FLAG_INIT_QP = 1,
//     SRMC_CREATE_FLAG_TGT_QP = 2,
// };

// int mlx5_ib_map_ubuf(struct mlx5_ib_sched* sched,unsigned long virt_addr,size_t size,int qpn,int cqn);
// int mlx5_ib_map_cq_ubuf(struct mlx5_ib_sched* sched,unsigned long virt_addr,size_t size,int cqn);
// int scheduler_polling(void* data);
// int mlx5_ib_create_srmc(struct mlx5_ib_sched* sched,struct mlx5_ib_qp *init_qp,struct mlx5_ib_qp *tgt_qp,union ib_gid *dgid);
// int mlx5_ib_sched_init(struct mlx5_ib_sched* sched);
// void mlx5_ib_sched_exit(struct mlx5_ib_sched* sched);
// //flags == 1: check ini, flags == 2: check tgt
// //If no exists, return -1 and create the srmc, if init qp then create kernel qp;else return 1 and add the refcnt of that qp
// int is_xrc_exists(struct mlx5_ib_sched* sched,struct ib_pd *pd,union ib_gid *dgid,int flags,int qpn);

// int mlx5_ib_unmap_ubuf(struct mlx5_ib_sched* sched,int qpn);
// int mlx5_ib_destroy_srmc(struct mlx5_ib_sched* sched,int ah_id);
// int mlx5_ib_create_srmc_qp(struct mlx5_ib_sched* sched,struct mlx5_ib_srmc *srmc,struct ib_pd *pd,int flags,int qpn);
// #endif /* _MLX5_IB_SCHEDULER_H */