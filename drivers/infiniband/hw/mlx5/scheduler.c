#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <asm/page_types.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <rdma/ib_verbs.h>
#include <rdma/mlx5-abi.h>
#include "mlx5_ib.h"
#include "scheduler.h"
#include "qp.h"
#include "wr.h"
#include <linux/mlx5/device.h>
#include <linux/err.h>
#include "user_verbs.h"
#include "conn.h"
#include <linux/inet.h>
#include <linux/jhash.h>
#include <rdma/ib_cm.h>
#include <linux/delay.h>
#include <linux/compiler.h>
#include <linux/random.h>
#include <linux/jiffies.h>

// 文件操作
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "mlx5_avl_tree.h"
#include <linux/mlx5/driver.h>

static struct aligned_u32 *user_wqe_table, *user_level_table;
struct xrc_table_entry ***user_xrc_table;//三维数组
// 用户态mmap表，表示当前多少个wqe已经下发
static struct page **user_wqe_pages, **user_level_pages, **user_xrc_pages;
uint32_t kernel_wqe_table[NUM_SQB], kernel_level_table[NUM_LEVEL * NUM_SCHED]; // 内核态表，表示当前srm qp中内核已发送多少个wqe
int num_table_qp, num_table_level,num_xrc_per_srm,num_user_threads, num_xrc_qp;
int num_table_apps;

struct ib_cq *shared_cq[NUM_SCHED][CQ_NUM]; // 每个内核线程一个cq,大小srmc各CQ_NUM个cq

uint16_t tot_xrc_sended_wqes[MAX_USER_THREADS_NUM][NUM_SCHED*NUM_LEVEL][MAX_USER_XRC_QP_PER_SRM],
            cur_xrc_sended_wqes[MAX_USER_THREADS_NUM][NUM_SCHED][MAX_USER_XRC_QP_PER_SRM]; // 统计0~4KB，每个用户线程每个xrc qp发送了多少wqe
uint64_t lst_xrc_bytes[MAX_USER_THREADS_NUM][NUM_SCHED][MAX_USER_XRC_QP_PER_SRM]; // 0~4KB,记录上次统计时每个xrc qp发送的总字节数
extern struct mlx5_uars_page *mlx5_get_uars_page_by_index(struct mlx5_core_dev *mdev,
                                                   int uar_index);

static inline uint32_t srm_load_level_total(int level, int level_table_bias)
{
    uint32_t sum = 0;
    int app;

    if (!user_level_table)
        return 0;

    if (num_table_apps <= 1)
        return smp_load_acquire(&user_level_table[level + level_table_bias].val);

    for (app = 0; app < num_table_apps; app++) {
        int idx = app * NUM_LEVEL * NUM_SCHED + level + level_table_bias;
        sum += smp_load_acquire(&user_level_table[idx].val);
    }

    return sum;
}

static inline void mlx5_sq_ctrl_set_cons_idx(struct mlx5_ib_sqbuf *sqb)
{
    if (sqb && sqb->ctrl)
        smp_store_release(&sqb->ctrl->cons_idx, sqb->cur_post);
}

static inline void mlx5_sq_ctrl_inc_cons_idx(struct mlx5_sq_ctrl_page *ctrl)
{
    u64 cons;

    if (!ctrl)
        return;

    cons = smp_load_acquire(&ctrl->cons_idx);
    smp_store_release(&ctrl->cons_idx, cons + 1);
}

static void mlx5_ib_free_srmc_ready_seq(struct mlx5_ib_srmc *srmc)
{
    u32 i;

    if (!srmc || !srmc->ready_seq_pages)
        return;

    for (i = 0; i < srmc->ready_seq_npages; i++)
        if (srmc->ready_seq_pages[i])
            put_page(srmc->ready_seq_pages[i]);

    kfree(srmc->ready_seq_pages);
    srmc->ready_seq_pages = NULL;
    srmc->ready_seq_npages = 0;
    srmc->ready_seq_depth = 0;
}

static void mlx5_ib_free_srmc_usr_rc(struct mlx5_ib_srmc *srmc)
{
    u32 i;

    if (!srmc || !srmc->usr_rc_pages)
        return;

    for (i = 0; i < srmc->usr_rc_npages; i++)
        if (srmc->usr_rc_pages[i])
            put_page(srmc->usr_rc_pages[i]);

    kfree(srmc->usr_rc_pages);
    srmc->usr_rc_pages = NULL;
    srmc->usr_rc_npages = 0;
    srmc->usr_rc_depth = 0;
}

static u32 mlx5_ib_srmc_get_usr_rc(struct mlx5_ib_srmc *srmc, u32 idx)
{
    size_t off;
    u32 page_idx;
    u32 page_off;
    u32 *entry;

    if (!srmc || !srmc->usr_rc_pages || !srmc->usr_rc_depth)
        return U32_MAX;

    idx &= srmc->usr_rc_depth - 1;
    off = (size_t)idx * sizeof(*entry);
    page_idx = off / PAGE_SIZE;
    page_off = off % PAGE_SIZE;
    if (page_idx >= srmc->usr_rc_npages || !srmc->usr_rc_pages[page_idx])
        return U32_MAX;

    entry = (u32 *)((char *)page_address(srmc->usr_rc_pages[page_idx]) + page_off);
    return READ_ONCE(*entry);
}

static struct mlx5_ib_srmc *mlx5_ib_sched_find_srmc_idx(struct mlx5_ib_sched *sched,
                                                        u32 srmc_idx)
{
    if (!sched)
        return NULL;
    if (srmc_idx >= NUM_SRMC)
        return NULL;

    return READ_ONCE(sched->srmc_by_idx[srmc_idx]);
}

static inline struct mlx5_sq_ctrl_page *mlx5_sq_ctrl_get_slot(
    struct mlx5_qp_ctrl_pool *pool, u32 slot_idx)
{
    size_t off;
    u32 page_idx;
    u32 page_off;

    if (!pool || !pool->pages)
        return NULL;
    if (slot_idx >= pool->slot_cnt)
        return NULL;

    off = (size_t)slot_idx * pool->slot_stride;
    page_idx = off / PAGE_SIZE;
    page_off = off % PAGE_SIZE;
    if (page_idx >= pool->npages || !pool->pages[page_idx])
        return NULL;

    return (struct mlx5_sq_ctrl_page *)((char *)page_address(
               pool->pages[page_idx]) + page_off);
}


                                                   
int mlx5_ib_map_ubuf(struct mlx5_ib_sched_group *sched_group, unsigned long virt_addr, size_t size, int qpn, int cqn, u32 uidx)
{
    DEBUG_LOG("in mlx5_ib_map_ubuf\n");
    DEBUG_LOG("内核态virt_addr:%px,size:%d,qpn:%d,cqn%d\n", virt_addr, size, qpn, cqn);
    struct mm_struct *mm = current->mm;
    int ret;
    int i;
    struct mlx5_ib_sqbuf *uq;
    size_t npages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    struct page **pages = kmalloc_array(npages, sizeof(struct page *), GFP_KERNEL);
    struct mlx5_ib_cqbuf *cqb;
    if (!pages)
        return -ENOMEM;
    //**pages参数存储的是物理页（Physical Page）的元数据描述符，而非虚拟地址或物理地址的直接数值
    // 可使用kmap(page)或vmap转换为内核态虚拟地址，page_to_phys(page)转换为物理地址
    ret = get_user_pages(virt_addr, npages, FOLL_WRITE, pages, NULL); // pin user pages in memory, Returns number of pages pinned.
    if (ret < npages)
    {
        // 如果获取的页面数少于预期，释放资源并返回错误
        for (i = 0; i < ret; i++)
            put_page(pages[i]);
        kfree(pages);
        return -EFAULT;
    }
    mutex_lock(&sched_group->sq_lock);
    uq = kzalloc(sizeof(struct mlx5_ib_sqbuf), GFP_KERNEL);
    uq->qpn = qpn;
    uq->uidx = uidx;
    uq->sq_size = size;
    uq->wqe_cnt = size / sizeof(struct srm_qp_entry);//改成srm_qp_entry的大小
    pr_info("DEBUG: sqb->wqe_cnt:%d\n", uq->wqe_cnt);
    uq->buf = vmap(pages, npages, VM_MAP, PAGE_KERNEL); // map the pages(phys addr) to kernel space（virtual addr），非连续物理页映射到虚拟页
    uq->pages = pages;
    if (!uq->buf)
    {
        for (i = 0; i < ret; i++)
            put_page(pages[i]);
        kfree(pages);
        kfree(uq);
        pr_err("fail to map sq buffer\n");
        mutex_unlock(&sched_group->sq_lock);
        return -ENOMEM;
    }



    sched_group->sqb_arr[sched_group->sqb_cnt] = uq;
    uq->idx = sched_group->sqb_cnt; // for debug only
    sched_group->sqb_cnt++;

    // pr_info("map sq buf %d success\n", sched_group->sqb_cnt - 1);
    mutex_unlock(&sched_group->sq_lock);

    return 0;
}

struct mlx5_uars_page *srm_get_user_uars_page(struct mlx5_ib_dev *dev, u32 uar_index)
{
    struct mlx5_core_dev *mdev = dev->mdev;
    struct mlx5_uars_page *up;

    up = mlx5_get_uars_page_by_index(mdev, uar_index);
    if (!up)
        return NULL;

    /* 这一页就是和用户共享的那一页：up->index == uar_index */
    return up;  /* 用完时记得 mlx5_put_uars_page(mdev, up) */
}

int srm_map_bf(struct mlx5_ib_sched_group *sched_group,struct mlx5_ib_create_qp *ucmd,struct mlx5_ib_dev *dev) {
    pr_info("DEBUG:in srm_map_bf,ucmd->bfreg_index:%d,ucmd->index_uar_in_page:%d\n",ucmd->bfreg_index,ucmd->index_uar_in_page);
    
    int ret ;
    int i;
    int db_found = 0;
    struct xrc_bf_entry *xrc_bf = kzalloc(sizeof(struct xrc_bf_entry), GFP_KERNEL);
    u64 kaddr = dev->mdev->bar_addr + ucmd->bfreg_index*PAGE_SIZE;
    xrc_bf->uar_page_vaddr = ioremap_wc(kaddr, MLX5_ADAPTER_PAGE_SIZE);
    if(!xrc_bf->uar_page_vaddr){
        pr_err("DEBUG: ioremap_wc failed\n");
        ret = -ENOMEM;
        goto err;
    }
    xrc_bf->bf_addr = xrc_bf->uar_page_vaddr
                        + MLX5_BF_OFFSET
                        + ucmd->index_in_uar * ucmd->db_bf_reg_size;
    xrc_bf->bf_size = ucmd->bf_buf_size;
    xrc_bf->bf_offset = ucmd->bf_offset;


   

    pr_info("DEBUG: bf_addr=%px, bf_size=%d, bf_offset=%d\n",
         xrc_bf->bf_addr, xrc_bf->bf_size, xrc_bf->bf_offset);
    sched_group->xrc_bf_arr[sched_group->xrc_bf_cnt++] = xrc_bf;
    return 0;
err:
    kfree(xrc_bf);
    return -ENOMEM;

}
int mlx5_ib_map_cq_ubuf(struct mlx5_ib_sched_group *sched_group, unsigned long virt_addr, size_t size, int cqn)
{
    DEBUG_LOG("in mlx5_ib_map_cq_ubuf\n");
    struct mm_struct *mm = current->mm;
    int ret;
    int i;
    struct mlx5_ib_cqbuf *uq;
    size_t npages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    struct page **pages = kmalloc_array(npages, sizeof(struct page *), GFP_KERNEL);
    if (!pages)
        return -ENOMEM;
    ret = get_user_pages(virt_addr, npages, FOLL_WRITE, pages, NULL);
    if (ret < npages)
    {
        // 如果获取的页面数少于预期，释放资源并返回错误
        for (i = 0; i < ret; i++)
            put_page(pages[i]);
        kfree(pages);
        return -EFAULT;
    }
    mutex_lock(&sched_group->cq_lock);
    uq = kzalloc(sizeof(struct mlx5_ib_cqbuf), GFP_KERNEL);
    uq->cqn = cqn;
    uq->cq_size = size;
    uq->buf = vmap(pages, npages, VM_MAP, PAGE_KERNEL);
    uq->pages = pages;
    uq->cqe_sz = 64;
    mutex_init(&uq->lock);
    if (!uq->buf)
    {
        kfree(uq);
        for (i = 0; i < ret; i++)
            put_page(pages[i]);
        kfree(pages);
        pr_err("failed to map cf buffer\n");
        mutex_unlock(&sched_group->cq_lock);
        return -ENOMEM;
    }
    sched_group->cqb_arr[sched_group->cqb_cnt] = uq;
    sched_group->cqb_cnt++;

    mutex_unlock(&sched_group->cq_lock);

    return 0;
}

static struct mlx5_ib_cqbuf *
mlx5_ib_find_cqb_by_cqn_locked(struct mlx5_ib_sched_group *sched_group, int cqn)
{
    int i;

    for (i = 0; i < sched_group->cqb_cnt; i++) {
        struct mlx5_ib_cqbuf *cqb = sched_group->cqb_arr[i];

        if (cqb && cqb->cqn == cqn)
            return cqb;
    }

    return NULL;
}

int mlx5_ib_bind_usr_rc_cq(struct mlx5_ib_sched_group *sched_group,
                           u32 usr_rc_cnt, int cqn)
{
    struct mlx5_ib_cqbuf *cqb;

    if (!sched_group)
        return -EINVAL;

    if (usr_rc_cnt >= ARRAY_SIZE(sched_group->usr_rc_cqb_arr))
        return -EINVAL;

    mutex_lock(&sched_group->cq_lock);
    cqb = mlx5_ib_find_cqb_by_cqn_locked(sched_group, cqn);
    if (!cqb) {
        mutex_unlock(&sched_group->cq_lock);
        pr_warn_ratelimited("hollow RC usr_rc_cnt %u cannot find mapped CQ cqn %d\n",
                            usr_rc_cnt, cqn);
        return -ENOENT;
    }

    sched_group->usr_rc_cqb_arr[usr_rc_cnt] = cqb;
    mutex_unlock(&sched_group->cq_lock);

    return 0;
}

struct mlx5_ib_sqbuf *mlx5_ib_find_sqbuf_by_qpn(struct mlx5_ib_sched_group *sched_group, int qpn)
{
    int i;

    for (i = 0; i < sched_group->sqb_cnt; i++) {
        struct mlx5_ib_sqbuf *sqb = sched_group->sqb_arr[i];

        if (sqb && sqb->qpn == qpn)
            return sqb;
    }

    return NULL;
}

int mlx5_ib_unmap_ubuf(struct mlx5_ib_sched_group *sched_group, int qpn)
{
    struct mlx5_ib_sqbuf *sqb;
    struct mlx5_ib_cqbuf *cqb;
    int cqn = -1;
    int npages;
    int i;
    mutex_lock(&sched_group->sq_lock);
    pr_info("mlx5_ib_unmap_ubuf清除映射资源\n");

    // Free sq buffer
    for (i = 0; i < sched_group->sqb_cnt; i++)
    {
        sqb = sched_group->sqb_arr[i];
        if (sqb == NULL)
            continue;
        if (sqb->qpn == qpn)
        {
            sched_group->sqb_arr[i] = NULL;
            if (sqb->cqb)
                cqn = sqb->cqb->cqn;
            vunmap(sqb->buf);
            npages = (sqb->sq_size + PAGE_SIZE - 1) / PAGE_SIZE;
            put_user_pages(sqb->pages, npages);
            kfree(sqb->pages);
            kfree(sqb);

            break;
        }
    }

    mutex_unlock(&sched_group->sq_lock);

    if (cqn < 0)
        return 0;

    // Free cq
    mutex_lock(&sched_group->cq_lock);

    for (i = 0; i < sched_group->cqb_cnt; i++)
    {
        cqb = sched_group->cqb_arr[i];

        if (cqb == NULL)
            continue;
        if (cqb->cqn == cqn)
        {
            int j;

            for (j = 0; j < ARRAY_SIZE(sched_group->usr_rc_cqb_arr); j++) {
                if (sched_group->usr_rc_cqb_arr[j] == cqb)
                    sched_group->usr_rc_cqb_arr[j] = NULL;
            }

            sched_group->cqb_arr[i] = NULL;
            vunmap(cqb->buf);
            npages = (cqb->cq_size + PAGE_SIZE - 1) / PAGE_SIZE;
            put_user_pages(cqb->pages, npages);
            kfree(cqb->pages);
            kfree(cqb);

            break;
        }
    }
    mutex_unlock(&sched_group->cq_lock);
    return 0;
}
static void print_wqe_info(void *seg, size_t size)
{
    // int exp_sz;

    // exp_sz = sizeof(struct mlx5_wqe_ctrl_seg) +
    // 		 sizeof(struct mlx5_wqe_xrc_seg) +
    // 		 sizeof(struct mlx5_wqe_raddr_seg) +
    // 		 sizeof(struct mlx5_wqe_data_seg);

    printk("size is %zu\n", size);

    // Parse and print the WQE segments
    struct mlx5_wqe_ctrl_seg *ctrl_seg = (struct mlx5_wqe_ctrl_seg *)seg;
    printk("Control Segment:\n");
    printk("  opmod_idx_opcode: 0x%x\n", ntohl(ctrl_seg->opmod_idx_opcode));
    printk("  qpn_ds: 0x%x\n", ntohl(ctrl_seg->qpn_ds));
    printk("  signature: 0x%x\n", ctrl_seg->signature);
    printk("  dci_stream_channel_id: 0x%x\n", ntohs(*(u16 *)ctrl_seg->rsvd));
    printk("  fm_ce_se: 0x%x\n", ctrl_seg->fm_ce_se);
    printk("  imm: 0x%x\n", ntohl(ctrl_seg->imm));

    struct mlx5_wqe_xrc_seg *xrc_seg = (struct mlx5_wqe_xrc_seg *)((char *)seg + sizeof(struct mlx5_wqe_ctrl_seg));
    printk("XRC Segment:\n");
    printk("  xrc_srqn: 0x%x\n", ntohl(xrc_seg->xrc_srqn));
    printk("  rsvd: 0x%x\n", xrc_seg->rsvd[0]);
    struct mlx5_wqe_raddr_seg *raddr_seg = (struct mlx5_wqe_raddr_seg *)((char *)xrc_seg + sizeof(struct mlx5_wqe_xrc_seg));
    printk("RADDR Segment:\n");
    printk("  raddr: 0x%lx\n", __be64_to_cpu(raddr_seg->raddr));
    printk("  rkey: 0x%x\n", ntohl(raddr_seg->rkey));

    struct mlx5_wqe_data_seg *data_seg = (struct mlx5_wqe_data_seg *)((char *)raddr_seg + sizeof(struct mlx5_wqe_raddr_seg));
    printk("Data Segment:\n");
    printk("  byte_count: 0x%x\n", ntohl(data_seg->byte_count));
    printk("  lkey: 0x%x\n", ntohl(data_seg->lkey));
    printk("  addr: 0x%lx\n", __be64_to_cpu(data_seg->addr));
}

// static inline uint64_t rdtsc(void)
// {
//     unsigned int lo, hi;
//     asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
//     return ((uint64_t)hi << 32) | lo;
// }

// 仅支持64B的标准wqe
static inline void srm_poll_once(struct mlx5_ib_sched *sched, struct ib_wc *wc, void **cqe)
{
    struct mlx5_ib_srmc *srmc;
    int cnt_c = 0;
    struct mlx5_ib_sqbuf *sqb;
    struct mlx5_ib_cqbuf *cqb;
    void *ucqe;
    int qpn;
    int cqn = -1;
    struct mlx5_cqe64 *ucqe64, *cqe64;
    int op_own;
    int uidx, idx;
    int cqe_num;
    int i, j;
    for (i = 0; i < NUM_SRMC; i++)
    {
        if (cnt_c >= sched->srmc_cnt)
        {
            break;
        }
        srmc = sched->srmc_tb[i];
        if (srmc == NULL)
        {
            continue;
        }
        cnt_c++;
        if (srmc->sig_cnt)
        {
            DEBUG_LOG("distributing cqe\n");
            // memset(&wc, 1, sizeof wc);
            cqe_num = 0;
            if ((cqe_num = mlx5_ib_poll_cq_with_cqe(srmc->ini_cb.cq, srmc->sig_cnt, wc, cqe)))
            {
                // 减去sig_cnt
                srmc->sig_cnt -= cqe_num;
                // cnt2++;
                for (j = 0; j < cqe_num; j++)
                {
                    // cqe64 = (to_mcq(srmc->ini_cb.qp->ibqp.send_cq)->mcq.cqe_sz == 64) ? cqe : cqe + 64;
                    //  Two attr to change
                    idx = wc[j].wr_id;
                    if (idx < 0 || idx >= SQ_DEPTH) {
                        pr_warn_ratelimited("invalid scheduler CQ info idx %d\n", idx);
                        continue;
                    }
                    // 减去发送中的字节数
                    srmc->pending_bytes -= srmc->wqe_infos[idx].pending_bytes;
                    if (srmc->wqe_infos[idx].to_user == 0)
                    {
                        continue;
                    }
                    // qpn = (wc.wr_id >> 32) & 0xffffff; // wr_id high 32 bits is qpn,low  32 bits is wqe_counter
                    qpn = srmc->wqe_infos[idx].qpn;
                    DEBUG_LOG("cqe_num:%d,wc status:%d,byte_cnt:%d\n", cqe_num, wc[j].status, srmc->wqe_infos[idx].pending_bytes);
                    DEBUG_LOG("wc's qpn:%d,wc's wqe_counter:%llu\n", qpn,
                              srmc->wqe_infos[idx].wqe_counter);
                    cqb = srmc->wqe_infos[idx].cqb;
                    if (cqb == NULL)
                    {
                        pr_err("Unexpected:No cqn found for qpn %d\n", qpn);
                        continue;
                    }
                    mutex_lock(&cqb->lock); // 多个线程可能同时写入同一个cq
                    //  distribute
                    //  TODO:change the owner bit
                    DEBUG_LOG("cqn:%d\n", cqb->cqn);
                    ucqe = cqb->buf + cqb->cur_put * cqb->cqe_sz;
                    ucqe64 = (cqb->cqe_sz == 64) ? ucqe : ucqe + 64;
                    cqe64 = cqe[j];
                    memcpy(ucqe, cqe[j], cqb->cqe_sz - 1);
                    DEBUG_LOG("cqe64->op_own:%x,cqe_size:%d\n", ucqe64->op_own, cqb->cqe_sz);
                    ucqe64->sop_drop_qpn = htonl(ntohl(ucqe64->sop_drop_qpn) & (~0xffffff) | qpn);
                    ucqe64->wqe_counter = htons(srmc->wqe_infos[idx].wqe_counter & 0xffff);
                    // 根据cqe v1，保存uidx
                    ucqe64->srqn = htonl(srmc->wqe_infos[idx].usr_rc_cnt);

                    // 反转用户态cqe的owner_bit
                    smp_store_release(&ucqe64->op_own, (cqe64->op_own & (~0xf)) | cqb->op_own);
                    DEBUG_LOG("ucqe64->op_own:%x,op_own:%d,cur_put:%d\n", ucqe64->op_own, cqb->op_own, cqb->cur_put);
                    cqb->cur_put++;
                    if ((cqb->cur_put << 6) >= cqb->cq_size)
                    {
                        cqb->cur_put = 0;
                        cqb->op_own ^= MLX5_CQE_OWNER_MASK;
                    }

                    mutex_unlock(&cqb->lock);
                    DEBUG_LOG("distribute cqe finished\n");
                }
            }
        }
    }
}
static inline int srm_poll_srmc_once(struct mlx5_ib_srmc *srmc, struct ib_wc *wc, void **cqe, int free_cqe_idx[], int *free_cqe_cnt)
{
    DEBUG_LOG("in srm_poll_srmc_once,sig_cnt:%d\n", srmc->sig_cnt);
    struct mlx5_ib_sqbuf *sqb;
    struct mlx5_ib_cqbuf *cqb;
    void *ucqe;
    int qpn;
    int cqn = -1;
    struct mlx5_cqe64 *ucqe64, *cqe64;
    int op_own;
    int uidx, idx;
    int cqe_num;
    int i, j;
    if (srmc->sig_cnt)
    {
        DEBUG_LOG("distributing cqe\n");

        // memset(&wc, 1, sizeof wc);
        cqe_num = 0;
        if ((cqe_num = mlx5_ib_poll_cq_with_cqe(srmc->ini_cb.cq, srmc->sig_cnt, wc, cqe)))
        {
            // 减去sig_cnt
            srmc->sig_cnt -= cqe_num;
            // pr_info("sig_cnt cqe_num:%d,sig_cnt:%d\n", cqe_num, srmc->sig_cnt);
            //  cnt2++;
            for (i = 0; i < cqe_num; i++)
            {
                // cqe64 = (to_mcq(srmc->ini_cb.qp->ibqp.send_cq)->mcq.cqe_sz == 64) ? cqe : cqe + 64;
                //  Two attr to change
                idx = wc[i].wr_id;
                if (idx < 0 || idx >= SQ_DEPTH) {
                    pr_warn_ratelimited("invalid scheduler CQ info idx %d\n", idx);
                    continue;
                }
                // 减去发送中的字节数
                //srmc->pending_bytes -= srmc->wqe_infos[idx].pending_bytes;
                if (srmc->wqe_infos[idx].to_user == 0)
                {
                    continue;
                }
                // qpn = (wc.wr_id >> 32) & 0xffffff; // wr_id high 32 bits is qpn,low  32 bits is wqe_counter
                //qpn = srmc->wqe_infos[idx].qpn;
                // DEBUG_LOG("cqe_num:%d,wc status:%d,byte_cnt:%d\n", cqe_num, wc[i].status, srmc->wqe_infos[idx].pending_bytes);
                // DEBUG_LOG("wc's qpn:%d,wc's wqe_counter:%llu\n", qpn,
                //           srmc->wqe_infos[idx].wqe_counter);
                cqb = srmc->wqe_infos[idx].cqb;
                if (cqb == NULL)
                {
                    pr_err("Unexpected:No cqn found for scheduler info idx %d\n", idx);
                    continue;
                }
                mutex_lock(&cqb->lock); // 多个线程可能同时写入同一个cq
                //  distribute
                //  TODO:change the owner bit
                DEBUG_LOG("cqn:%d\n", cqb->cqn);
                ucqe = cqb->buf + cqb->cur_put * cqb->cqe_sz;
                ucqe64 = (cqb->cqe_sz == 64) ? ucqe : ucqe + 64;
                cqe64 = cqe[i];
                memcpy(ucqe, cqe[i], cqb->cqe_sz - 1);
                DEBUG_LOG("cqe64->op_own:%x,cqe_size:%d\n", ucqe64->op_own, cqb->cqe_sz);
                //ucqe64->sop_drop_qpn = htonl(ntohl(ucqe64->sop_drop_qpn) & (~0xffffff) | qpn);
                //ucqe64->wqe_counter = htons(srmc->wqe_infos[idx].wqe_counter & 0xffff);
                // 根据cqe v1，保存uidx
                // ucqe64->srqn = htonl(sqb->uidx);

                // 反转用户态cqe的owner_bit
                smp_store_release(&ucqe64->op_own, (cqe64->op_own & (~0xf)) | cqb->op_own);
                DEBUG_LOG("ucqe64->op_own:%x,op_own:%d,cur_put:%d\n", ucqe64->op_own, cqb->op_own, cqb->cur_put);
                cqb->cur_put++;
                if ((cqb->cur_put << 6) >= cqb->cq_size)
                {
                    cqb->cur_put = 0;
                    cqb->op_own ^= MLX5_CQE_OWNER_MASK;
                }

                mutex_unlock(&cqb->lock);
                // pr_info("distribute cqe finished\n");
                mlx5_sq_ctrl_inc_cons_idx(srmc->wqe_infos[idx].ctrl_page);
                srmc->wqe_infos[idx].valid = 0;

                free_cqe_idx[*free_cqe_cnt] = idx;
                (*free_cqe_cnt)++;
            }
        }

        return cqe_num;
    }
    else
    {
        return -1;
    }
}

static inline int srm_poll_srmc_once_debug(struct mlx5_ib_srmc *srmc, struct ib_wc *wc, void **cqe, struct file *filp,
                                           loff_t *pos, char *buf, uint64_t *start_cycles, uint64_t *end_cycles, int free_cqe_idx[], int *free_cqe_cnt)
{
    DEBUG_LOG("in srm_poll_srmc_once,sig_cnt:%d\n", srmc->sig_cnt);
    struct mlx5_ib_sqbuf *sqb;
    struct mlx5_ib_cqbuf *cqb;
    void *ucqe;
    int qpn;
    int cqn = -1;
    struct mlx5_cqe64 *ucqe64, *cqe64;
    int op_own;
    int uidx, idx;
    int cqe_num;
    int i, j;
    int len;
    int ret;

    uint64_t elapsed_ns, elapsed_cycles;

    const uint64_t cpu_frequency_hz = 2900000000; // 2.9 GHz
    if (srmc->sig_cnt)
    {
        DEBUG_LOG("distributing cqe\n");

        // memset(&wc, 1, sizeof wc);
        cqe_num = 0;
        if ((cqe_num = mlx5_ib_poll_cq_with_cqe(srmc->ini_cb.cq, srmc->sig_cnt, wc, cqe)))
        {
            *end_cycles = rdtsc();
            elapsed_cycles = *end_cycles - *start_cycles;
            elapsed_ns = (elapsed_cycles * 1000000000) / cpu_frequency_hz;

            // 减去sig_cnt
            srmc->sig_cnt -= cqe_num;
            // cnt2++;
            for (i = 0; i < cqe_num; i++)
            {
                // cqe64 = (to_mcq(srmc->ini_cb.qp->ibqp.send_cq)->mcq.cqe_sz == 64) ? cqe : cqe + 64;
                //  Two attr to change
                idx = wc[i].wr_id;
                if (idx < 0 || idx >= SQ_DEPTH) {
                    pr_warn_ratelimited("invalid scheduler CQ info idx %d\n", idx);
                    continue;
                }
                // 减去发送中的字节数
                srmc->pending_bytes -= srmc->wqe_infos[idx].pending_bytes;
                if (srmc->wqe_infos[idx].to_user == 0)
                {
                    continue;
                }
                // qpn = (wc.wr_id >> 32) & 0xffffff; // wr_id high 32 bits is qpn,low  32 bits is wqe_counter
                qpn = srmc->wqe_infos[idx].qpn;
                DEBUG_LOG("cqe_num:%d,wc status:%d,byte_cnt:%d\n", cqe_num, wc[i].status, srmc->wqe_infos[idx].pending_bytes);
                DEBUG_LOG("wc's qpn:%d,wc's wqe_counter:%llu\n", qpn,
                          srmc->wqe_infos[idx].wqe_counter);
                cqb = srmc->wqe_infos[idx].cqb;
                if (cqb == NULL)
                {
                    pr_err("Unexpected:No cqn found for qpn %d\n", qpn);
                    continue;
                }
                mutex_lock(&cqb->lock); // 多个线程可能同时写入同一个cq
                //  distribute
                //  TODO:change the owner bit
                DEBUG_LOG("cqn:%d\n", cqb->cqn);
                ucqe = cqb->buf + cqb->cur_put * cqb->cqe_sz;
                ucqe64 = (cqb->cqe_sz == 64) ? ucqe : ucqe + 64;
                cqe64 = cqe[i];
                memcpy(ucqe, cqe[i], cqb->cqe_sz - 1);
                DEBUG_LOG("cqe64->op_own:%x,cqe_size:%d\n", ucqe64->op_own, cqb->cqe_sz);
                ucqe64->sop_drop_qpn = htonl(ntohl(ucqe64->sop_drop_qpn) & (~0xffffff) | qpn);
                ucqe64->wqe_counter = htons(srmc->wqe_infos[idx].wqe_counter & 0xffff);
                // 根据cqe v1，保存uidx
                ucqe64->srqn = htonl(srmc->wqe_infos[idx].usr_rc_cnt);

                // 反转用户态cqe的owner_bit
                smp_store_release(&ucqe64->op_own, (cqe64->op_own & (~0xf)) | cqb->op_own);
                DEBUG_LOG("ucqe64->op_own:%x,op_own:%d,cur_put:%d\n", ucqe64->op_own, cqb->op_own, cqb->cur_put);
                cqb->cur_put++;
                if ((cqb->cur_put << 6) >= cqb->cq_size)
                {
                    cqb->cur_put = 0;
                    cqb->op_own ^= MLX5_CQE_OWNER_MASK;
                }

                mutex_unlock(&cqb->lock);
                DEBUG_LOG("distribute cqe finished\n");

                /* 3. 写数据 */
                len = scnprintf(buf, 256, "polled user cqe,k:%d,byte_cnt:%d,elapsed time from last poll:%llu(ns)\n", srmc->srmc_idx, srmc->wqe_infos[idx].byte_cnt, elapsed_ns);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
                /* kernel_write 从 5.11+ 内核可用，无需 set_fs */
                ret = kernel_write(filp, buf, len, pos);
#else
                ret = vfs_write(filp, buf, len, pos);
#endif
                if (ret < 0)
                    pr_err("write_int_to_file: write error %d\n", ret);

                mlx5_sq_ctrl_inc_cons_idx(srmc->wqe_infos[idx].ctrl_page);
                srmc->wqe_infos[idx].valid = 0;

                free_cqe_idx[*free_cqe_cnt] = idx;
                (*free_cqe_cnt)++;
            }

            *start_cycles = rdtsc();
        }
        else
        {
            /* 3. 写数据 */
            len = scnprintf(buf, 256, "can't poll anything,now sig_cnt:%d\n", srmc->sig_cnt);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
            /* kernel_write 从 5.11+ 内核可用，无需 set_fs */
            ret = kernel_write(filp, buf, len, pos);
#else
            ret = vfs_write(filp, buf, len, pos);
#endif
            if (ret < 0)
                pr_err("write_int_to_file: write error %d\n", ret);
        }
        return cqe_num;
    }
    else
    {
        /* 3. 写数据 */
        len = scnprintf(buf, 256, "can't poll anything,now sig_cnt:%d\n", srmc->sig_cnt);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
        /* kernel_write 从 5.11+ 内核可用，无需 set_fs */
        ret = kernel_write(filp, buf, len, pos);
#else
        ret = vfs_write(filp, buf, len, pos);
#endif
        if (ret < 0)
            pr_err("write_int_to_file: write error %d\n", ret);
        return -1;
    }
}

static inline uint32_t srm_fastrand(uint64_t *seed)
{
    *seed = *seed * 1103515245 + 12345;
    return (uint32_t)((*seed) >> 32);
}

static int has_wqes(struct mlx5_ib_sqbuf *sqb, int id)
{

    int uidx;
    struct mlx5_wqe_ctrl_seg *uctrl;
    u32 imm;
    uidx = sqb->cur_post & (sqb->wqe_cnt - 1);
    uctrl = (sqb->buf + (uidx << 6));    // 64B的wqe
    imm = smp_load_acquire(&uctrl->imm); // 内存屏障，为1表示有wr
    return imm && id == ((imm >> 24) & 0xFF);
}
static int mod_add(int a, int b, int mod)
{
    return (a + b + mod) % mod;
}

static int calc_level_tot_wqe_num(int n, int num_user_threads)
{
    int ret;
    int i;
    ret = 0;
    for (i = n / num_user_threads * num_user_threads; i <= (n / num_user_threads + 1) * num_user_threads; i++)
    {
        ret += user_wqe_table[i].val - kernel_wqe_table[i];
    }
    return ret;
}

const int num_kqps = 32;

static __always_inline int poll_srmc_inline(
    struct mlx5_ib_srmc **pre_srmcs,
    int *polling_tail,
    int *polling_head,
    u8 *in_queue,
    struct ib_wc *wc,
    void **cqe,
    int *free_cqe_idx,
    int *free_cqe_cnt,
    int id,
    struct mlx5_ib_srmc *srmc)
{
    // 从队列尾取出SRMC
    struct mlx5_ib_srmc *pre_srmc = pre_srmcs[*polling_tail];
    int polled = 0;
    int ret;

    pre_srmcs[*polling_tail] = NULL; // 清空当前位置

    if (!pre_srmc)
        return 0; // 无待处理的SRMC，直接返回

    // 移动尾指针（位运算替代模运算，假设SRMC_POLLING_CNT是2的幂）
    *polling_tail = (*polling_tail + 1) & (SRMC_POLLING_CNT - 1);

    // 执行一次poll操作
    ret = srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, free_cqe_cnt);
    if (ret > 0)
        polled += ret;

    // 处理poll后仍有未完成的sig_cnt
    if (pre_srmc->sig_cnt)
    {
        // 情况1：轮询队列已满，必须处理完所有剩余信号
        if (unlikely(pre_srmcs[*polling_head] != NULL))
        {
            pr_info("[sched-%d] cq polling queue exceed queue length\n", id);
            while (pre_srmc->sig_cnt)
            {
                ret = srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, free_cqe_cnt);
                if (ret > 0)
                    polled += ret;
            }
            in_queue[pre_srmc->srmc_idx & (CQ_NUM - 1)] = 0; // 位运算优化
        }
        // 情况2：sig_cnt超过SQ_DEPTH或SQ队列满，需处理至安全范围
        else if (unlikely(pre_srmc->sig_cnt >= pre_srmc->ini_cb.qp->sq.wqe_cnt ||
                          (int)(pre_srmc->ini_cb.qp->sq.head -
                                pre_srmc->ini_cb.qp->sq.tail) >=
                              pre_srmc->ini_cb.qp->sq.max_post))
        {
            if (pre_srmc->sig_cnt >= pre_srmc->ini_cb.qp->sq.wqe_cnt)
            {
                pr_info("[sched-%d] cq queue exceed qp depth\n", id);
            }
            else
            {
                pr_info("[sched-%d] exceed max_post00, sq.head:%d, sq.tail:%d, max_post:%d\n",
                        id, pre_srmc->ini_cb.qp->sq.head,
                        pre_srmc->ini_cb.qp->sq.tail,
                        pre_srmc->ini_cb.qp->sq.max_post);
            }
            // 循环处理至sig_cnt < SQ_DEPTH且SQ不满
            while (pre_srmc->sig_cnt >= pre_srmc->ini_cb.qp->sq.wqe_cnt ||
                   (pre_srmc->ini_cb.qp->sq.head -
                    pre_srmc->ini_cb.qp->sq.tail) >=
                       pre_srmc->ini_cb.qp->sq.max_post)
            {
                ret = srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, free_cqe_cnt);
                if (ret > 0)
                    polled += ret;
            }
            // 处理完成后判断是否仍有剩余信号
            if (!pre_srmc->sig_cnt)
            {
                in_queue[pre_srmc->srmc_idx & (CQ_NUM - 1)] = 0; // 位运算优化
            }
            else
            {
                pre_srmcs[*polling_head] = pre_srmc;
                *polling_head = (*polling_head + 1) & (SRMC_POLLING_CNT - 1); // 位运算优化
            }
        }
        // 情况3：仍有剩余信号但队列未满，重新加入队列
        else
        {
            pre_srmcs[*polling_head] = pre_srmc;
            *polling_head = (*polling_head + 1) & (SRMC_POLLING_CNT - 1); // 位运算优化
        }
    }
    // 无剩余信号，标记为出队
    else
    {
        in_queue[pre_srmc->srmc_idx & (CQ_NUM - 1)] = 0; // 位运算优化
    }

    return polled;
}


static __always_inline struct mlx5_ib_srmc *find_target_srmc(struct mlx5_ib_sched *sched, const union ib_gid *gid,
                                                             uint32_t length, uint32_t *hash_id, uint64_t *srm_seed,
                                                             int id)
{
    struct mlx5_ib_srmc *srmc = NULL;
    int i;

    // 哈希查找匹配GID的SRMC
    for (i = 0; i < NUM_SRMC; i++)
    {
        int j = (i + (*hash_id)) % NUM_SRMC;
        srmc = sched->srmc_tb[j];
        if (!srmc)
        {
            pr_err("Unexpected:No srmc found for this wr\n");
            continue;
        }

        if (memcmp(srmc->dgid.raw, gid->raw, sizeof(gid->raw)) == 0)
        {
            DEBUG_LOG("found srmc[%d] (gid.if_id=%llx)", j, srmc->dgid.global.interface_id);
            if (!srmc->ini_cb.qp)
            {
                pr_err_once("[id=%d] srmc[%d] qp is NULL", id, j);
                return NULL;
            }

            // 随机选择一个SRMC（负载均衡）
            uint32_t rd = prandom_u32_max(num_kqps);
            j = (j + rd) % NUM_SRMC;
            srmc = sched->srmc_tb[j];
            return srmc;
        }
    }

    pr_err_once("[id=%d] no srmc found for gid (length=%d)", id, length);
    return NULL;
}

static inline uint64_t calc_time(uint64_t start_cycles, uint64_t end_cycles){

    const uint64_t cpu_frequency_hz = 2900000000;
    uint64_t elapsed_cycles, elapsed_ns;
    elapsed_cycles = end_cycles - start_cycles;
    elapsed_ns = (elapsed_cycles * 1000000000) / cpu_frequency_hz;
    return elapsed_ns;
}

void srm_doorbell(struct xrc_bf_entry *bf, uint64_t ctrl,uint16_t cur_post){
    if (unlikely(!bf)) {
        pr_warn_ratelimited("srm_doorbell: bf is NULL, ctrl=%llx cur_post=%u\n",
            (unsigned long long)ctrl, cur_post);
        return;
    }
    //pr_info("in srm_doorbell\n");
    //wmb();
    //pr_info("bf->db.kaddr:%px\n",bf->db.kaddr);
	//bf->db.kaddr[1] = cpu_to_be32(cur_post);

	/* Make sure doorbell record is visible to the HCA before
	 * we hit doorbell.
	 */
	wmb();
    mlx5_write64((__be32 *)&ctrl, bf->bf_addr + bf->bf_offset);
    bf->bf_offset ^= bf->bf_size;
    wmb();
}

static inline uint64_t calc_tot_cqes(int num_user_threads){
    int i;
    uint64_t res = 0;
    for(i = 0;i<num_user_threads;i++){
        res += smp_load_acquire(&user_xrc_table[i][0][0].tot_recv_cqes);
    }
    return res;
}
static inline void log_limit_batch_update(const char *phase,
                                          u64 old_limit,
                                          u64 new_limit,
                                          u64 prev_bw,
                                          u64 prev_lat,
                                          u64 cur_bw,
                                          u64 cur_lat)
{
    pr_info("limit_batch %s: limit %llu->%llu, bw %llu->%llu Gbps, lat %llu->%llu us\n",
            phase,
            (unsigned long long)old_limit,
            (unsigned long long)new_limit,
            (unsigned long long)prev_bw,
            (unsigned long long)cur_bw,
            (unsigned long long)prev_lat,
            (unsigned long long)cur_lat);
}

static inline void update_limit_batch(struct mlx5_ib_sched *sched){
    enum {
        STATE_STARTUP = 0,
        STATE_SEARCH = 1,
    };
    enum {
        PROBE_NONE = 0,
        PROBE_UP = 1,
        PROBE_DOWN = 2,
    };
    struct srm_sample {
        u64 bw;
        u64 lat;
        int valid;
    };

    static int state = STATE_STARTUP;
    static int probe_state = PROBE_NONE;
    static int search_base_limit;
    static int search_step;
    static struct srm_sample upper_sample;
    static struct srm_sample lower_sample;
    static u64 pre_bw;
    static u64 pre_lat;

    static u64 last_bw_val[MAX_USER_THREADS_NUM];
    static u64 last_lat_val[MAX_USER_THREADS_NUM];
    static u64 last_bw_cnt[MAX_USER_THREADS_NUM];
    static u64 last_lat_cnt[MAX_USER_THREADS_NUM];
    static u64 pending_bw;
    static u64 pending_lat;
    static int pending_bw_valid;
    static int pending_lat_valid;
    static u64 last_applied_bw;
    static u64 last_applied_lat;

    static u64 startup_updates_seen;
    static unsigned long startup_ignore_until;
    const int min_batch = 1;
    const int max_batch = 100000;
    const u32 bw_eps_pct = 1;  /* 2% */
    const u32 lat_eps_pct = 5; /* 5% */

    const u32 startup_ignore_updates = 2;
    const unsigned long startup_ignore_secs = 5;
    const u64 startup_lat_target_us = 50;
    int t;
    int step;
    int limit;
    u64 avg_gbps, avg_lat;

    /* 只在一个调度线程中执行自适应逻辑，避免多线程竞争 */
    if (!sched || sched->id != 0)
        return;

    /* 没有注册 xrc_table 或用户线程信息时不做调整 */
    if (!user_xrc_table || !num_user_threads || !num_xrc_per_srm)
        return;

    if (num_user_threads > MAX_USER_THREADS_NUM)
        return;

    /* 控制调整频率，避免过于频繁地读取共享表 */
    {
        static unsigned long last_jiffies;
        const unsigned long interval = HZ * 10;

        if (time_before(jiffies, last_jiffies + interval))
            return;
        last_jiffies = jiffies;
    }

    /*
     * 吞吐和时延由不同线程更新：分别捕获“新吞吐”和“新时延”。
     * 条件：update_cnt 稳定 + 该指标值发生变化。
     */
    for (t = 0; t < num_user_threads; t++) {
        u64 cnt1, cnt2;
        u64 bw, lat;

        cnt1 = smp_load_acquire(&user_xrc_table[t][0][0].update_cnt);
        bw = smp_load_acquire(&user_xrc_table[t][0][0].cur_Gbps);
        lat = smp_load_acquire(&user_xrc_table[t][0][0].cur_lat_us);

        if (!cnt1)
            continue;

        if (bw && cnt1 != last_bw_cnt[t] ) {
            last_bw_cnt[t] = cnt1;
            last_bw_val[t] = bw;
            pending_bw = bw;
            pending_bw_valid = 1;
        }

        if (lat && cnt1 != last_lat_cnt[t] ) {
            last_lat_cnt[t] = cnt1;
            last_lat_val[t] = lat;
            pending_lat = lat;
            pending_lat_valid = 1;
        }
    }

    if (!pending_bw_valid || !pending_lat_valid){
        pr_info_ratelimited("pending_bw_valid:%d,pending_lat_valid:%d\n", pending_bw_valid, pending_lat_valid);
        return ;
    }

    avg_gbps = pending_bw;
    avg_lat = pending_lat;
    pending_bw_valid = 0;
    pending_lat_valid = 0;

    if (state == STATE_STARTUP) {
        if (startup_ignore_updates || startup_ignore_secs) {
            if (!startup_ignore_until && startup_ignore_secs)
                startup_ignore_until = jiffies + startup_ignore_secs * HZ;

            startup_updates_seen++;

            if ((startup_ignore_updates && startup_updates_seen <= startup_ignore_updates) ||
                (startup_ignore_secs && time_before(jiffies, startup_ignore_until)))
                return;
        }
    }

    limit = LIMIT_BATCHING;
    if (limit < min_batch)
        limit = min_batch;
    if (limit > max_batch)
        limit = max_batch;

    step = limit / 20;
    if (step < 1)
        step = 1;

    if (state == STATE_STARTUP) {
        if (!pre_bw || !pre_lat) {
            pre_bw = avg_gbps;
            pre_lat = avg_lat;
            limit = min(limit * 2, max_batch);
            log_limit_batch_update("startup-init",
                                   LIMIT_BATCHING,
                                   limit,
                                   last_applied_bw,
                                   last_applied_lat,
                                   avg_gbps,
                                   avg_lat);
            LIMIT_BATCHING = limit;
            last_applied_bw = avg_gbps;
            last_applied_lat = avg_lat;
            return;
        }

        if (avg_gbps * 100 > pre_bw * (100 + bw_eps_pct) &&
            avg_lat <= startup_lat_target_us) {
            limit = min(limit * 2, max_batch);
        } else {
            limit = max(limit / 2, min_batch);
            state = STATE_SEARCH;
            probe_state = PROBE_NONE;
            search_base_limit = limit;
            upper_sample.valid = 0;
            lower_sample.valid = 0;
        }

        pre_bw = avg_gbps;
        pre_lat = avg_lat;
        log_limit_batch_update("startup",
                               LIMIT_BATCHING,
                               limit,
                               last_applied_bw,
                               last_applied_lat,
                               avg_gbps,
                               avg_lat);
        LIMIT_BATCHING = limit;
        last_applied_bw = avg_gbps;
        last_applied_lat = avg_lat;
        return;
    }

    /* SEARCH: 轮流探测 upper / lower，并比较选择更优 */
    if (state == STATE_SEARCH) {
        int upper_limit;
        int lower_limit;

        if (probe_state == PROBE_NONE) {
            search_base_limit = limit;
            search_step = step;
            upper_sample.valid = 0;
            lower_sample.valid = 0;
        }

        upper_limit = min(search_base_limit + search_step, max_batch);
        lower_limit = max(search_base_limit - search_step, min_batch);

        if (probe_state == PROBE_NONE) {
            if (upper_limit != search_base_limit) {
                probe_state = PROBE_UP;
                log_limit_batch_update("search-probe-up",
                                       LIMIT_BATCHING,
                                       upper_limit,
                                       last_applied_bw,
                                       last_applied_lat,
                                       avg_gbps,
                                       avg_lat);
                LIMIT_BATCHING = upper_limit;
                last_applied_bw = avg_gbps;
                last_applied_lat = avg_lat;
                return;
            }
            if (lower_limit != search_base_limit) {
                probe_state = PROBE_DOWN;
                log_limit_batch_update("search-probe-down",
                                       LIMIT_BATCHING,
                                       lower_limit,
                                       last_applied_bw,
                                       last_applied_lat,
                                       avg_gbps,
                                       avg_lat);
                LIMIT_BATCHING = lower_limit;
                last_applied_bw = avg_gbps;
                last_applied_lat = avg_lat;
                return;
            }
            return;
        }

        if (probe_state == PROBE_UP) {
            upper_sample.bw = avg_gbps;
            upper_sample.lat = avg_lat;
            upper_sample.valid = 1;

            if (lower_limit != search_base_limit) {
                probe_state = PROBE_DOWN;
                log_limit_batch_update("search-probe-down",
                                       LIMIT_BATCHING,
                                       lower_limit,
                                       last_applied_bw,
                                       last_applied_lat,
                                       avg_gbps,
                                       avg_lat);
                LIMIT_BATCHING = lower_limit;
                last_applied_bw = avg_gbps;
                last_applied_lat = avg_lat;
                return;
            }

            limit = upper_limit;
            probe_state = PROBE_NONE;
            log_limit_batch_update("search-commit-up",
                                   LIMIT_BATCHING,
                                   limit,
                                   last_applied_bw,
                                   last_applied_lat,
                                   avg_gbps,
                                   avg_lat);
            LIMIT_BATCHING = limit;
            last_applied_bw = avg_gbps;
            last_applied_lat = avg_lat;
            return;
        }

        if (probe_state == PROBE_DOWN) {
            lower_sample.bw = avg_gbps;
            lower_sample.lat = avg_lat;
            lower_sample.valid = 1;

            if (upper_sample.valid && lower_sample.valid) {
                if (upper_sample.bw * 100 > lower_sample.bw * (100 + bw_eps_pct)) {
                    limit = upper_limit;
                } else if (lower_sample.bw * 100 > upper_sample.bw * (100 + bw_eps_pct)) {
                    limit = lower_limit;
                } else if (upper_sample.lat * 100 <
                           lower_sample.lat * (100 - lat_eps_pct)) {
                    limit = upper_limit;
                } else {
                    limit = lower_limit;
                }
            } else if (lower_sample.valid) {
                limit = lower_limit;
            }

            probe_state = PROBE_NONE;
            log_limit_batch_update("search-commit",
                                   LIMIT_BATCHING,
                                   limit,
                                   last_applied_bw,
                                   last_applied_lat,
                                   avg_gbps,
                                   avg_lat);
            LIMIT_BATCHING = limit;
            last_applied_bw = avg_gbps;
            last_applied_lat = avg_lat;
            return;
        }
    }
}

// //文件，记录db_cycles数据
// uint64_t db_cpu_cycles[5000000],polling_cpu_cycles[5000000];
// int db_cycles_cnt = 0,polling_cycles_cnt = 0;

static inline uint64_t fc_sum(uint64_t *arr, int n){
    uint64_t sum = 0;
    int i;
    for(i = 0;i<n;i++){
        sum += arr[i];
    }
    return sum;
}

/*
 * 在阻塞等待 inflight 降到阈值期间，执行一次“时延线程”调度。
 * 该路径复用主调度中的发送与元数据更新逻辑，避免时延线程被纯 cpu_relax 饿死。
 */
static __always_inline bool srm_poll_latency_once(
    struct mlx5_ib_sched_group *sched_group,
    int id,
    int current_num_user_threads,
    int current_table_qp,
    int num_thread_qps,
    int level_table_bias,
    uint32_t real_num_threads,
    int wqes_limit_sz,
    uint64_t *wqe_tot_sz,
    uint64_t *wqe_ewma_sz,
    uint64_t cur_wqes[WQES_ARR_SZ],
    int *wqe_cur_idx,
    uint64_t alpha_a,
    uint64_t alpha_b)
{
    int polling_order[][NUM_LEVEL] = {
        {0, 1},
        {1, 0}
    };
    int64_t target_sz;
    int order_idx;
    int l, n, k;
    int level;
    int latency_thread_idx;
    uint32_t user_level_val;
    uint32_t level_wqe_cnt;
    uint32_t user_table_val, kernel_table_val;

    if (!current_num_user_threads)
        return false;

    latency_thread_idx = (int)real_num_threads - 1;
    if (latency_thread_idx < 0 || latency_thread_idx >= current_num_user_threads)
        return false;

    target_sz = wqes_limit_sz - *wqe_ewma_sz;
    if (target_sz <= 10240)
        order_idx = 0;
    else
        order_idx = 1;

    for (l = 0; l < NUM_LEVEL; l++) {
        level = polling_order[order_idx][l];

        user_level_val = srm_load_level_total(level, level_table_bias);
        if (unlikely(user_level_val < kernel_level_table[level + level_table_bias])) {
            kernel_level_table[level + level_table_bias] = user_level_val;
            continue;
        }
        level_wqe_cnt = user_level_val - kernel_level_table[level + level_table_bias];
        if (!level_wqe_cnt)
            continue;

        n = latency_thread_idx * NUM_LEVEL * NUM_SCHED + level * NUM_SCHED + id;
        user_table_val = smp_load_acquire(&user_wqe_table[n].val);
        kernel_table_val = kernel_wqe_table[n];

        if (unlikely(user_table_val < kernel_table_val)) {
            kernel_wqe_table[n] = user_table_val;
            continue;
        }

        if (user_table_val == kernel_table_val)
            continue;

        k = level * sched_group->num_sched + id + latency_thread_idx * num_thread_qps;
        if (unlikely(k >= current_table_qp))
            continue;

        struct mlx5_ib_sqbuf *sqb = sched_group->sqb_arr[k];
        if (!sqb)
            continue;

        if (level == 0) {
            for (; !kthread_should_stop();) {
                if (user_table_val == kernel_wqe_table[n])
                    break;

                struct srm_qp_entry *qp_entry =
                    (struct srm_qp_entry *)sqb->buf + (sqb->cur_post & (sqb->wqe_cnt - 1));
                uint32_t srm_qp_valid = smp_load_acquire(&qp_entry->valid);
                if (!srm_qp_valid) {
                    pr_warn_ratelimited("latency poll: level=0 qp entry invalid, resync k:%d n:%d user_wqe:%u kernel_wqe:%u\n",
                                        k, n, user_wqe_table[n].val, kernel_wqe_table[n]);
                    kernel_wqe_table[n] = user_table_val;
                    kernel_level_table[level + level_table_bias] = user_level_val;
                    if (likely(sqb->wqe_cnt))
                        sqb->cur_post = user_table_val & (sqb->wqe_cnt - 1);
                    break;
                }

                int xrc_qp_idx = qp_entry->qp_idx;
                if (unlikely(xrc_qp_idx < 0 || xrc_qp_idx >= num_xrc_per_srm)) {
                    pr_warn_ratelimited("latency poll: xrc_qp_idx OOB idx=%d num_xrc_per_srm=%d n=%d k=%d\n",
                                        xrc_qp_idx, num_xrc_per_srm, n, k);
                    kernel_wqe_table[n] = user_table_val;
                    if (likely(sqb->wqe_cnt))
                        sqb->cur_post = user_table_val & (sqb->wqe_cnt - 1);
                    mlx5_sq_ctrl_set_cons_idx(sqb);
                    break;
                }

                if (tot_xrc_sended_wqes[latency_thread_idx][NUM_SCHED * level + id][xrc_qp_idx] !=
                    cur_xrc_sended_wqes[latency_thread_idx][id][xrc_qp_idx]) {
                    cur_xrc_sended_wqes[latency_thread_idx][id][xrc_qp_idx]++;
                    sqb->cur_post++;
                    mlx5_sq_ctrl_set_cons_idx(sqb);
                    smp_store_release(&qp_entry->valid, 0);
                    kernel_wqe_table[n]++;
                    kernel_level_table[level + level_table_bias]++;
                    cpu_relax();
                    continue;
                }

                uint64_t xrc_ctrl = smp_load_acquire(
                    &user_xrc_table[latency_thread_idx][NUM_SCHED * level + id][xrc_qp_idx].ctrl);
                uint64_t xrc_tot_bytes = smp_load_acquire(
                    &user_xrc_table[latency_thread_idx][NUM_SCHED * level + id][xrc_qp_idx].tot_bytes) -
                    lst_xrc_bytes[latency_thread_idx][id][xrc_qp_idx];

                lst_xrc_bytes[latency_thread_idx][id][xrc_qp_idx] += xrc_tot_bytes;

                uint32_t opmod_idx_opcode = be32_to_cpu((uint32_t)xrc_ctrl);
                uint16_t new_tot_wqes = ((opmod_idx_opcode >> 8) & 0xffff) + 1;
                kernel_wqe_table[n]++;
                kernel_level_table[level + level_table_bias]++;
                tot_xrc_sended_wqes[latency_thread_idx][level * NUM_SCHED + id][xrc_qp_idx] = new_tot_wqes;
                cur_xrc_sended_wqes[latency_thread_idx][id][xrc_qp_idx]++;
                sqb->cur_post++;
                mlx5_sq_ctrl_set_cons_idx(sqb);

                {
                    // int bf_idx = latency_thread_idx * NUM_SCHED * NUM_LEVEL * num_xrc_per_srm +
                    //              (level * NUM_SCHED + id) * num_xrc_per_srm +
                    //              xrc_qp_idx;
                    int bf_idx = latency_thread_idx * NUM_SCHED * NUM_LEVEL * num_xrc_per_srm +
                                  NUM_LEVEL * NUM_SCHED * xrc_qp_idx + level * NUM_SCHED + id;
                    srm_doorbell(sched_group->xrc_bf_arr[bf_idx], xrc_ctrl, new_tot_wqes - 1);
                }

                smp_store_release(&qp_entry->cycles, rdtsc());

                *wqe_tot_sz -= cur_wqes[*wqe_cur_idx];
                *wqe_tot_sz += xrc_tot_bytes;
                cur_wqes[*wqe_cur_idx] = xrc_tot_bytes;
                *wqe_cur_idx = (*wqe_cur_idx + 1) % WQES_ARR_SZ;
                *wqe_ewma_sz = ((alpha_b - alpha_a) * (*wqe_ewma_sz) + alpha_a * (*wqe_tot_sz)) / alpha_b;

                smp_store_release(&qp_entry->valid, 0);

                return true;
            }
        } else {
            struct srm_qp_entry *qp_entry =
                (struct srm_qp_entry *)sqb->buf + (sqb->cur_post & (sqb->wqe_cnt - 1));
            uint32_t srm_qp_valid = smp_load_acquire(&qp_entry->valid);
            if (!srm_qp_valid) {
                pr_warn_ratelimited("latency poll: level>0 qp entry invalid, resync k:%d n:%d user_wqe:%u kernel_wqe:%u\n",
                                    k, n, user_wqe_table[n].val, kernel_wqe_table[n]);
                kernel_wqe_table[n] = user_table_val;
                kernel_level_table[level + level_table_bias] = user_level_val;
                if (likely(sqb->wqe_cnt))
                    sqb->cur_post = user_table_val & (sqb->wqe_cnt - 1);
                continue;
            }

            int xrc_qp_idx = qp_entry->qp_idx;
            if (unlikely(xrc_qp_idx < 0 || xrc_qp_idx >= num_xrc_per_srm)) {
                pr_warn_ratelimited("latency poll: xrc_qp_idx OOB idx=%d num_xrc_per_srm=%d n=%d k=%d\n",
                                    xrc_qp_idx, num_xrc_per_srm, n, k);
                kernel_wqe_table[n] = user_table_val;
                if (likely(sqb->wqe_cnt))
                    sqb->cur_post = user_table_val & (sqb->wqe_cnt - 1);
                mlx5_sq_ctrl_set_cons_idx(sqb);
                continue;
            }

            smp_store_release(&qp_entry->valid, 0);

            {
                uint64_t xrc_ctrl = qp_entry->ctrl;
                uint64_t cur_bytes = qp_entry->bytes;

                sqb->cur_post++;
                mlx5_sq_ctrl_set_cons_idx(sqb);

                {
                                
                    // int bf_idx = latency_thread_idx * NUM_SCHED * NUM_LEVEL * num_xrc_per_srm +
                    //              (level * NUM_SCHED + id) * num_xrc_per_srm +
                    //              xrc_qp_idx;
                    int bf_idx = latency_thread_idx * NUM_SCHED * NUM_LEVEL * num_xrc_per_srm +
                                 NUM_LEVEL * NUM_SCHED * xrc_qp_idx + level * NUM_SCHED + id;
                    srm_doorbell(sched_group->xrc_bf_arr[bf_idx],
                                 xrc_ctrl,
                                 tot_xrc_sended_wqes[latency_thread_idx][level * NUM_SCHED + id][xrc_qp_idx]);
                }

                smp_store_release(&qp_entry->cycles, rdtsc());

                *wqe_tot_sz -= cur_wqes[*wqe_cur_idx];
                *wqe_tot_sz += cur_bytes;
                cur_wqes[*wqe_cur_idx] = cur_bytes;
                *wqe_cur_idx = (*wqe_cur_idx + 1) % WQES_ARR_SZ;

                kernel_wqe_table[n]++;
                kernel_level_table[level + level_table_bias]++;
                tot_xrc_sended_wqes[latency_thread_idx][level * NUM_SCHED + id][xrc_qp_idx]++;
                *wqe_ewma_sz = ((alpha_b - alpha_a) * (*wqe_ewma_sz) + alpha_a * (*wqe_tot_sz)) / alpha_b;

                return true;
            }
        }
    }

    return false;
}

int scheduler_polling(void *sched_data)
{
    extern struct mlx5_ib_sched_group sched_group;
    int ret;
    struct mlx5_ib_sched_id *sched_id = (struct mlx5_ib_sched_id *)sched_data;
    struct mlx5_ib_sched *sched = sched_id->sched;
    int id = sched_id->id;
    struct mlx5_ib_sqbuf *sqb;
    struct mlx5_ib_cqbuf *cqb;
    struct mlx5_ib_srmc *srmc;
    struct ib_wc *wc;
    void **cqe, *ucqe;
    int qpn;
    int op_own;
    int uidx, idx;
    int i, j, k;

    void *seg, *useg;
    struct mlx5_wqe_ctrl_seg *ctrl, *uctrl;
    struct mlx5_wqe_raddr_seg *raddr, *uraddr;
    struct mlx5_wqe_data_seg *data, *udata;
    struct mlx5_wqe_xrc_seg *xrc, *uxrc;
    int length;
    struct mlx5_ib_qp *qp;
    union ib_gid gid;
    unsigned long flags;
    u32 mlx5_opcode;
    u32 opmod;
    u32 imm;
    void *cur_edge;
    int hash_id;
    u8 next_fence;
    u8 fence;
    u8 sig;

    u8 to_user;

    int found;
    u32 rd;

    uint64_t start_cycles, end_cycles, elapsed_cycles;
    uint64_t elapsed_ns;

    uint64_t db_st_cycles,db_ed_cycles,db_elapsed_cycles;
    uint64_t db_elapsed_ns;
    uint64_t start_cycles_cq, end_cycles_cq;
    const uint64_t cpu_frequency_hz = 2900000000; // 2.9 GHz

    cqe = kmalloc_array(SQ_DEPTH, sizeof(void *), GFP_KERNEL);
    wc = kmalloc_array(SQ_DEPTH, sizeof(struct ib_wc), GFP_KERNEL);

    memset(gid.raw, 0, sizeof(gid.raw));
    memset(gid.raw + 10, 0xff, 2); // 高80位为0，中16位全1，低32位为ip地址，此为gid格式

    unsigned long tfree = 1, cnt = 0, cnt_c;
    // int cnt3 = 0;
    kfree(sched_id);

//     // 文件统计
//     char pt[200] = {0};
//     // snprintf(pt, 200, "/root/zxm/rdma-kerndriver/%ddata%d.txt", num_kqps, id);
//     snprintf(pt, 200, "/root/zxm/rdma-kerndriver/fcscale_log.txt");

//     struct file *filp;
//     loff_t pos = 0;
//     char *buf;
//     int len;
// #if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
//     mm_segment_t oldfs;
// #endif

//     /* 1. 准备字符串缓冲区 */
//     buf = kmalloc(256, GFP_KERNEL);
//     if (!buf)
//         return -ENOMEM;

//     /* 2. 打开（或创建）目标文件 */
// #if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
//     /* 小于 5.11 的内核需要 set_fs 才能访问文件系统 */
//     oldfs = get_fs();
//     set_fs(KERNEL_DS);
// #endif
//     filp = filp_open(pt,
//                         O_WRONLY | O_CREAT | O_TRUNC,
//                         0644);
// #if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
//     set_fs(oldfs);
// #endif
//     if (IS_ERR(filp))
//     {
//         ret = PTR_ERR(filp);
//         pr_info("Error open file\n");
//     }

    // 随机数序列固定种子
    uint64_t srm_seed;
    srm_seed = 0xdeadbeef;
    start_cycles = 0;
    start_cycles_cq = 0;

    uint32_t poll_round = 0;
    const int POLL_ALL_INTERVAL = 10000; // 全体遍历的时间
    struct mlx5_ib_srmc *pre_srmc = NULL;
    struct mlx5_ib_srmc **pre_srmcs = kmalloc_array(SRMC_POLLING_CNT, sizeof(struct mlx5_ib_srmc *), GFP_KERNEL);
    memset(pre_srmcs, 0, SRMC_POLLING_CNT * sizeof(struct mlx5_ib_srmc *));
    int polling_tail, polling_head;
    polling_tail = polling_head = 0;

    u8 *in_queue;
    in_queue = kmalloc_array(NUM_SRMC * 2, sizeof(u8), GFP_KERNEL); // 大小要是num_kqps的4倍
    memset(in_queue, 0, NUM_SRMC * 2 * sizeof(u8));

    int wqe_cur_idx = 0;







    struct mlx5_ib_srmc *cq_srmc_tb[CQ_NUM] = {0}; // 保存每个cq对应srmc代表

    int *free_cqe_idx, free_cqe_cnt;
    free_cqe_idx = kmalloc_array(SQ_DEPTH, sizeof(int), GFP_KERNEL);
    free_cqe_cnt = SQ_DEPTH;
    for (i = 0; i < SQ_DEPTH; i++)
    {
        free_cqe_idx[i] = i;
    }



    uint32_t level_wqe_cnt, wqe_cnt, user_threads_idx;
    int sending_case; // 对应新的wqe个数和旧的wqe个数的几种情况,0~2代表三种情况，3代表应该break了
    int *level_qp_st_arr = NULL;

    while (!kthread_should_stop())
    {
        ret = wait_event_interruptible(sched->init_wait,
            kthread_should_stop() ||
            READ_ONCE(sched->init_error) ||
            smp_load_acquire(&sched->ready_srmc_cnt) >= num_kqps);
        if (ret)
            continue;
        if (kthread_should_stop())
            goto out;
        if (READ_ONCE(sched->init_error)) {
            pr_err("scheduler thread %d: SRMC init failed: %d\n",
                   id, READ_ONCE(sched->init_error));
            goto out;
        }
        break;
    }

    level_qp_st_arr = kmalloc_array(NUM_LEVEL, sizeof(int), GFP_KERNEL);
    memset(level_qp_st_arr, 0, NUM_LEVEL * sizeof(int));

    u8 use_user_idx;

    uint64_t skip_cnt10 = 0, skip_cnt100 = 0, empty_rolling10 = 0, empty_rolling100 = 0;
    uint64_t wqe_sending_target_cnt = 10240;

    int level_table_bias = NUM_LEVEL * id;
    int user_thread_idx;
    int cq_idx;

    int wqe_idx_queue[1024]; // TODO:需要根据用户态传下来的bucket大小信息动态分配
    int idx_queue_cnt = 0;

    const int aggr_limit = 1; // 单次聚合的qp数量上限
    int t_cnt_idx,t_cnt_hash = 0;
    uint64_t roll_cnt = 0;

    uint64_t kernel_tot_db = 0, user_tot_cqes = 0;
    uint64_t inflight = 0;
    
    uint32_t stuck_cnt = 0;
    uint32_t lat_cnt = 0;

    uint32_t real_num_threads = 17; 
    struct mlx5_qp_ctrl_pool *sq_ctrl_pool = NULL;

    while (!kthread_should_stop())
    {

        if (!sq_ctrl_pool) {
            for (j = 0; j < NUM_SRMC; j++) {
                struct mlx5_ib_srmc *tmp = sched->srmc_tb[j];

                if (tmp && tmp->ini_cb.qp) {
                    if (tmp->ini_cb.qp->sq_ctrl_entry)
                        sq_ctrl_pool =
                            tmp->ini_cb.qp->sq_ctrl_entry->pool;
                    if (!sq_ctrl_pool) {
                        struct mlx5_ib_dev *dev =
                            to_mdev(tmp->ini_cb.qp->ibqp.device);
                        sq_ctrl_pool = dev ? &dev->sq_ctrl_pool : NULL;
                    }
                    break;
                }
            }
        }

        for (i = 0; i < num_kqps; i++) {


            if (cnt % 1000000 == 0)
            {
                cnt++;
                msleep(0);
            }
            ret = poll_srmc_inline(pre_srmc, &polling_tail,&polling_head,in_queue,wc,cqe,
                free_cqe_idx,&free_cqe_cnt,0,srmc);
            if (ret > 0)
                user_tot_cqes += ret;

            struct mlx5_sq_ctrl_page *ctrl_page;
            u64 pub;
            u64 credit;

            ctrl_page = mlx5_sq_ctrl_get_slot(sq_ctrl_pool, i);
            if (!ctrl_page)
                continue;
            if (!smp_load_acquire(&ctrl_page->wqe_cnt))
                continue;

            pub = smp_load_acquire(&ctrl_page->pub_idx);
            srmc = mlx5_ib_sched_find_srmc_idx(sched, i);
            if (!srmc || !srmc->ini_cb.qp)
                continue;

            stuck_cnt = 0;
            inflight = kernel_tot_db - user_tot_cqes;

            while (inflight >= LIMIT_BATCHING && !kthread_should_stop()) {
                ret = poll_srmc_inline(pre_srmc, &polling_tail, &polling_head,
                                       in_queue, wc, cqe, free_cqe_idx,
                                       &free_cqe_cnt, 0, srmc);
                if (ret > 0) {
                    user_tot_cqes += ret;
                    stuck_cnt = 0;
                } else {
                    cpu_relax();
                    if (++stuck_cnt % 1024 == 0)
                        cond_resched();
                }
                inflight = kernel_tot_db - user_tot_cqes;
            }
            if (kthread_should_stop())
                goto out;

            pub = smp_load_acquire(&ctrl_page->pub_idx);
            if (pub <= srmc->ini_cb.qp->sq.cur_post) {
                cnt++;
                continue;
            }

            inflight = kernel_tot_db - user_tot_cqes;
            if (inflight >= LIMIT_BATCHING)
                continue;
            credit = LIMIT_BATCHING - inflight;
            if (!credit)
                continue;

            if (free_cqe_cnt <= 0)
            {
                pr_err("free_cqe_cnt <= 0, cq exceed\n");
                goto err;
            }

            cq_idx = srmc->srmc_idx % (CQ_NUM);
            if (cq_srmc_tb[cq_idx] == NULL)
                cq_srmc_tb[cq_idx] = srmc;

            pre_srmc = cq_srmc_tb[cq_idx];
            if (unlikely(!pre_srmc)) {
                pr_warn_ratelimited("no CQ representative for srmc %d\n",
                                    srmc->srmc_idx);
                continue;
            }

            {
                u64 ready_cnt = pub - srmc->ini_cb.qp->sq.cur_post;
                int batch = min_t(u64, ready_cnt,
                                  min_t(u64, free_cqe_cnt, credit));
                int sent = 0;
                struct mlx5_wqe_ctrl_seg *last_ctrl = NULL;

                while (sent < batch) {
                    uint32_t usr_rc_cnt;

                    idx = srmc->ini_cb.qp->sq.cur_post &
                          (srmc->ini_cb.qp->sq.wqe_cnt - 1);
                    ctrl = mlx5_frag_buf_get_wqe(&srmc->ini_cb.qp->sq.fbc,
                                                  idx);
                    usr_rc_cnt = mlx5_ib_srmc_get_usr_rc(srmc, idx);

                    if (unlikely(usr_rc_cnt >= ARRAY_SIZE(sched_group.usr_rc_cqb_arr) ||
                                 !sched_group.usr_rc_cqb_arr[usr_rc_cnt])) {
                        pr_warn_ratelimited("invalid hollow RC id %u for srmc %d\n",
                                            usr_rc_cnt, srmc->srmc_idx);
                        break;
                    }

                    uidx = free_cqe_idx[--free_cqe_cnt];
                    pre_srmc->wqe_infos[uidx].qpn = usr_rc_cnt;
                    pre_srmc->wqe_infos[uidx].usr_rc_cnt = usr_rc_cnt;
                    pre_srmc->wqe_infos[uidx].wqe_counter =
                        srmc->ini_cb.qp->sq.cur_post;
                    pre_srmc->wqe_infos[uidx].cqb =
                        sched_group.usr_rc_cqb_arr[usr_rc_cnt];
                    pre_srmc->wqe_infos[uidx].to_user = 1;
                    pre_srmc->wqe_infos[uidx].valid = 1;
                    pre_srmc->wqe_infos[uidx].ctrl_page = ctrl_page;

                    srmc->ini_cb.qp->sq.wrid[idx] = uidx;
                    srmc->ini_cb.qp->sq.cur_post++;
                    last_ctrl = ctrl;
                    sent++;
                }

                if (!sent)
                    continue;

                mlx5r_ring_db(srmc->ini_cb.qp, sent, last_ctrl);
                kernel_tot_db += sent;

                if (!in_queue[cq_idx])
                {
                    if (pre_srmcs[polling_head] != NULL)
                    {
                        pre_srmc = pre_srmcs[polling_tail];
                        pre_srmcs[polling_tail] = NULL;
                        polling_tail = (polling_tail + 1) % SRMC_POLLING_CNT;
                        pr_info("err:exceed queue length\n");
                        if (pre_srmc) {
                            while ((ret = srm_poll_srmc_once(pre_srmc, wc, cqe,
                                                             free_cqe_idx,
                                                             &free_cqe_cnt)) != -1)
                            {
                                if (ret > 0)
                                    user_tot_cqes += ret;
                            }
                            in_queue[pre_srmc->srmc_idx % (CQ_NUM)] = 0;
                        }
                    }

                    pre_srmcs[polling_head] = cq_srmc_tb[cq_idx];
                    polling_head = (polling_head + 1) % SRMC_POLLING_CNT;
                    in_queue[cq_idx] = 1;
                }

                pre_srmc = cq_srmc_tb[cq_idx];
                DEBUG_LOG("send signaled\n");
                pre_srmc->sig_cnt += sent;
            }
            // pre_srmc->cur_cqe++;
        }
    }
out:
    DEBUG_LOG("scheduler thread %d exit\n", id);
    kfree(cqe);
    kfree(wc);
    kfree(pre_srmcs);
    kfree(in_queue);
    kfree(free_cqe_idx);
    kfree(level_qp_st_arr);

    // // 文件
    // filp_close(filp, NULL);
    // kfree(buf);
    return 0;
err:
    pr_err("scheduler thread %d exit in error state\n", id);
    kfree(cqe);
    kfree(wc);
    kfree(pre_srmcs);
    kfree(in_queue);
    kfree(free_cqe_idx);
    kfree(level_qp_st_arr);

    // // 文件
    // filp_close(filp, NULL);
    // kfree(buf);
    return -1;
}

int mlx5_ib_sched_init(struct mlx5_ib_sched_group *sched_group, int num)
{
    int i;
    int j;
    int ret;
    struct mlx5_ib_sched_id *sched_id;

    i = j = 0;

    mutex_init(&sched_group->sq_lock);
    mutex_init(&sched_group->cq_lock);

    /* Fresh scheduler lifecycle state */
    sched_group->sqb_cnt = 0;
    sched_group->cqb_cnt = 0;
    sched_group->xrc_bf_cnt = 0;
    memset(sched_group->sqb_arr, 0, sizeof(sched_group->sqb_arr));
    memset(sched_group->cqb_arr, 0, sizeof(sched_group->cqb_arr));
    memset(sched_group->usr_rc_cqb_arr, 0,
           sizeof(sched_group->usr_rc_cqb_arr));
    memset(sched_group->xrc_bf_arr, 0, sizeof(sched_group->xrc_bf_arr));

    sched_group->num_sched = num;
    sched_group->scheds = kzalloc(num * sizeof(struct mlx5_ib_sched), GFP_KERNEL);
    if (sched_group->scheds == NULL)
    {
        pr_err("Failed to allocate memory for sched_group\n");
        return -ENOMEM;
    }

    char thread_info[64];
    for (i = 0; i < num; i++)
    {

        snprintf(thread_info, sizeof(thread_info), "sched_thread_%d", i);

        sched_id = kzalloc(sizeof(struct mlx5_ib_sched_id), GFP_KERNEL);
        if (sched_id == NULL)
        {
            pr_err("Failed to allocate memory for sched_id\n");
            ret = -ENOMEM;
            goto err;
        }
        sched_id->sched = &sched_group->scheds[i];
        sched_group->scheds[i].id = i;
        sched_id->id = i;
        memset(sched_group->scheds[i].srmc_tb, 0,
               sizeof(sched_group->scheds[i].srmc_tb));
        memset(sched_group->scheds[i].srmc_by_idx, 0,
               sizeof(sched_group->scheds[i].srmc_by_idx));
        sched_group->scheds[i].srmc_cnt = 0;
        sched_group->scheds[i].ready_srmc_cnt = 0;
        sched_group->scheds[i].init_error = 0;
        init_waitqueue_head(&sched_group->scheds[i].init_wait);
        sched_group->scheds[i].task = kthread_create(scheduler_polling, (void *)sched_id, thread_info);
        mutex_init(&sched_group->scheds[i].srmc_lock);

        if (IS_ERR(sched_group->scheds[i].task))
        {
            pr_err("Failed to create polling thread%d\n", i);
            ret = PTR_ERR(sched_group->scheds[i].task);
            goto err;
        }
        kthread_bind(sched_group->scheds[i].task, i + 8);
        wake_up_process(sched_group->scheds[i].task);
        pr_info("Polling thread %d started and bound to CPU %d\n", i, i + 8);
    }
    // sched_group->cq_task =  kthread_create(polling_cqe,NULL,"polling_cqe");
    // if (IS_ERR(sched_group->cq_task))
    // {
    //     pr_err("Failed to create cqe polling thread\n");
    //     ret = PTR_ERR(sched_group->cq_task);
    //     goto err;
    // }
    // kthread_bind(sched_group->cq_task, 10);
    // wake_up_process(sched_group->cq_task);
    // pr_info("CQ polling thread started and bound to CPU %d\n", 10);
    return 0;
err:
    for (j = 0; j < i; j++)
    {
        if (sched_group->scheds[j].task)
        {
            kthread_stop(sched_group->scheds[j].task);
            sched_group->scheds[j].task = NULL;
        }
    }
    kfree(sched_group->scheds);
    return ret;
}
//TODO:释放bf（mlx5_put_uars_page(mdev, up)）
void mlx5_ib_sched_exit(struct mlx5_ib_sched_group *sched_group)
{
    struct mlx5_ib_sqbuf *sqb;
    struct mlx5_ib_cqbuf *cqb;
    struct mlx5_ib_srmc *srmc;
    struct mlx5_ib_sched *sched;
    int npages;
    int i, j,k;

    if (!sched_group || !sched_group->scheds || sched_group->num_sched <= 0) {
        pr_warn("mlx5_sched_exit: already cleaned or uninitialized\n");
        return;
    }

    for (i = 0; i < sched_group->num_sched; i++)
    {
        DEBUG_LOG("Ready to stop sched->task %d\n", i);
        sched = &sched_group->scheds[i];
        if (sched->task && !IS_ERR(sched->task) && pid_alive(sched->task))
        {
            kthread_stop(sched->task);
            sched->task = NULL;
        }
        else
        {
            sched->task = NULL;
        }
    }

    /*
     * Scheduler threads are stopped; safe to release SQ/CQ buffers pinned
     * with get_user_pages().
     */
    for (i = 0; i < sched_group->sqb_cnt; i++) {
        sqb = sched_group->sqb_arr[i];
        if (!sqb)
            continue;
        mlx5_ib_unmap_ubuf(sched_group, sqb->qpn);
    }

    for (i = 0; i < sched_group->num_sched; i++) {
        sched = &sched_group->scheds[i];
        mutex_lock(&sched->srmc_lock);
        for (j = 0; j < NUM_SRMC; j++) {
            srmc = sched->srmc_tb[j];
            mlx5_ib_free_srmc_ready_seq(srmc);
            mlx5_ib_free_srmc_usr_rc(srmc);
        }
        memset(sched->srmc_by_idx, 0, sizeof(sched->srmc_by_idx));
        mutex_unlock(&sched->srmc_lock);
    }
    //     mutex_lock(&sched->srmc_lock);
    //     for (j = 0; j < NUM_SRMC; j++)
    //     {
    //         srmc = sched->srmc_small_tb[j];
    //         if (srmc == NULL)
    //         {
    //             continue;
    //         }
    //         DEBUG_LOG("srmc ini_cb state:%d\n", srmc->ini_cb.state);
    //         // if (srmc->ini_cb.state == CONNECTED)//可能在event处理过程中发现state是CONNECTED，造成重复释放。如何解决？
    //         // {
    //         //     rdma_disconnect(srmc->ini_cb.cm_id);
    //         //     // ib_sched_free_buf(&srmc->ini_cb);
    //         //     ib_destroy_qp(&srmc->ini_cb.qp->ibqp);
    //         //     ib_destroy_cq(srmc->ini_cb.cq);
    //         // }
    //         if (srmc->ini_cb.cm_id)
    //         {
    //             rdma_disconnect(srmc->ini_cb.cm_id);
    //             ib_destroy_qp(&srmc->ini_cb.qp->ibqp);
    //             // ib_destroy_cq(srmc->ini_cb.cq);
    //             //  ib_dealloc_pd(srmc->ini_cb.pd);
    //             rdma_destroy_id(srmc->ini_cb.cm_id);
    //         }

    //         kfree(srmc);
    //     }

    //     for (j = 0; j < NUM_SRMC; j++)
    //     {
    //         srmc = sched->srmc_large_tb[j];
    //         if (srmc == NULL)
    //         {
    //             continue;
    //         }
    //         // if (srmc->ini_cb.state == CONNECTED)
    //         // {
    //         //     rdma_disconnect(srmc->ini_cb.cm_id);
    //         //     // ib_sched_free_buf(&srmc->ini_cb);
    //         //     ib_destroy_qp(&srmc->ini_cb.qp->ibqp);
    //         //     ib_destroy_cq(srmc->ini_cb.cq);
    //         // }
    //         if (srmc->ini_cb.cm_id)
    //         {
    //             rdma_disconnect(srmc->ini_cb.cm_id);
    //             ib_destroy_qp(&srmc->ini_cb.qp->ibqp);
    //             // ib_destroy_cq(srmc->ini_cb.cq);
    //             //  ib_dealloc_pd(srmc->ini_cb.pd);
    //             rdma_destroy_id(srmc->ini_cb.cm_id);
    //         }
    //         kfree(srmc);
    //     }
    //     mutex_unlock(&sched->srmc_lock);
    //     DEBUG_LOG("clean thread %d srmc success\n", i);
    // }

    // for (i = 0; i < sched_group->num_sched; i++)
    // {
    //     for (j = 0; j < (CQ_NUM); j++)
    //         // free cq
    //         if (shared_cq[i][j])
    //             ib_destroy_cq(shared_cq[i][j]);
    // }

    // clean up srm qp table
    if (user_wqe_table)
    {
        if (user_wqe_pages) {
            npages = ((num_table_qp * sizeof(user_wqe_table[0])) + PAGE_SIZE - 1) / PAGE_SIZE;
            vunmap(user_wqe_table);
            put_user_pages(user_wqe_pages, npages);
            kfree(user_wqe_pages);
            user_wqe_pages = NULL;
        }
        user_wqe_table = NULL;
    }
    pr_info("clean wqe table success\n");

    if (user_level_table)
    {
        if (user_level_pages) {
            npages = ((num_table_level * sizeof(user_level_table[0])) + PAGE_SIZE - 1) / PAGE_SIZE;
            vunmap(user_level_table);
            put_user_pages(user_level_pages, npages);
            kfree(user_level_pages);
            user_level_pages = NULL;
        }
        user_level_table = NULL;
    }
    pr_info("clean level table success\n");

    if(user_xrc_table){
        if (user_xrc_pages) {
            npages = ((num_xrc_qp * sizeof(user_xrc_table[0][0][0])) + PAGE_SIZE - 1) / PAGE_SIZE;
            vunmap(user_xrc_table[0][0]);
            put_user_pages(user_xrc_pages, npages);
            kfree(user_xrc_pages);
            user_xrc_pages = NULL;
        }
        user_xrc_table = NULL;
    }
    pr_info("clean xrc table success\n");



    struct xrc_bf_entry *bf;
    for(i = 0;i<sched_group->xrc_bf_cnt;i++){
        bf = sched_group->xrc_bf_arr[i];
        if(bf == NULL){
            continue;
        }
        iounmap(bf->uar_page_vaddr);
        // if(bf->db.page){
        //     vunmap((uint64_t)bf->db.kaddr & PAGE_MASK);
        //     put_page(bf->db.page);
        // }
        kfree(bf);
    }

    sched_group->xrc_bf_cnt = 0;

    kfree(sched_group->scheds);
    sched_group->scheds = NULL;
    sched_group->num_sched = 0;

    pr_info("mlx5_sched_exit success\n");
    // mlx5_ib_unmap_ubuf(sched,0);
}
int mlx5_ib_server_init(struct mlx5_ib_server *server)
{
    server->task = kthread_run(mlx5_sched_run_server, &server->server_cb, "server thread");
    if (IS_ERR(server->task))
    {
        DEBUG_LOG("Failed to create server thread\n");
        return PTR_ERR(server->task);
    }
    return 0;
}
void mlx5_ib_server_exit(struct mlx5_ib_server *server, struct mlx5_ib_sched_group *sched_group)
{
    int i, j;
    struct mlx5_ib_sched *sched;
    struct mlx5_ib_srmc *srmc;

    if (server->task)
    {
        kthread_stop(server->task);
        for (i = 0; i < sched_group->num_sched; i++)
        {
            sched = &sched_group->scheds[i];
            mutex_lock(&sched->srmc_lock);
            for (j = 0; j < NUM_SRMC; j++)
            {
                srmc = sched->srmc_tb[j];
                if (srmc == NULL)
                {
                    continue;
                }
                if (srmc->tgt_cb.cm_id)
                {
                    DEBUG_LOG("Freeing tgt cb's cm connection resources.\n");
                    rdma_disconnect(srmc->tgt_cb.cm_id);
                    ib_destroy_qp(&srmc->tgt_cb.qp->ibqp);
                    ib_destroy_cq(srmc->tgt_cb.cq);
                    ib_dealloc_pd(srmc->tgt_cb.pd);
                    rdma_destroy_id(srmc->tgt_cb.cm_id);
                }
            }
            mutex_unlock(&sched->srmc_lock);
        }
        if (server->server_cb.cm_id)
            rdma_destroy_id(server->server_cb.cm_id);
    }
    else
        pr_info("server task PTR is err\n");
    pr_info("mlx5_ib_server_exit success\n");
}
void mlx5_ib_gid2ip(char addr[4], union ib_gid *gid)
{
    memcpy(addr, gid->raw + 12, 4);
}
// return 0 means xrc exists, other means xrc not exists
int is_xrc_exists(struct mlx5_ib_sched *sched, struct ib_pd *pd, union ib_gid *dgid, int flags, int qpn, u32 sq_depth)
{

    DEBUG_LOG("in is_xrc_exists,gid.in_id = %llx, gid.subnet = %llx\n", dgid->global.interface_id, dgid->global.subnet_prefix);
    DEBUG_LOG("gid.raw[15]:%u\n", dgid->raw[15]);
    struct mlx5_ib_srmc *srmc;
    int ret = 1;
    int i, j;
    int hash_id;
    int has_srmc = 0;
    u32 depth = sq_depth ? sq_depth : SQ_DEPTH;

    if (depth > SQ_DEPTH)
        return -EINVAL;
    hash_id = sched_hash_ip((char *)dgid->raw + 12, NUM_SRMC);
    mutex_lock(&sched->srmc_lock);
    for (i = 0; i < NUM_SRMC; i++)
    {
        j = (hash_id + i) % NUM_SRMC;
        srmc = sched->srmc_tb[j];
        if (srmc == NULL)
        {
            break;
        }
	        if (memcmp(srmc->dgid.raw, dgid->raw, sizeof(srmc->dgid.raw)) == 0)
	        {
	            if (flags == SRMC_CREATE_FLAG_INIT_QP)
	                ret = 0;
	            has_srmc = 1;
	            break;
	        }
    }

	    if (!has_srmc)
	    {
	        if (sched->srmc_tb[j] != NULL)
	        {
	            pr_err("srmc queue is full\n");
            mutex_unlock(&sched->srmc_lock);
            return -1;
        }
        for (i = 0; i < num_kqps; i++)
        {
            // srmc no exists
            srmc = kzalloc(sizeof(struct mlx5_ib_srmc), GFP_KERNEL);
            if (!srmc) {
                ret = -ENOMEM;
                WRITE_ONCE(sched->init_error, ret);
                wake_up_all(&sched->init_wait);
                break;
            }
            memcpy(srmc->dgid.raw, dgid->raw, sizeof(srmc->dgid.raw));
            if (flags == SRMC_CREATE_FLAG_INIT_QP)
                srmc->ini_cb.refcnt = 0;
            srmc->idx = j;
            srmc->srmc_idx = i;

            sched->srmc_tb[j] = srmc;
            sched->srmc_cnt++;

            mutex_unlock(&sched->srmc_lock);
            ret = create_srmc_qp_cm(srmc, pd, dgid, MESSAGE_SIZE_SMALL, sched->id, depth);
            pr_info("create_srmc_qp_cm ret:%d\n", ret);
            mutex_lock(&sched->srmc_lock);
            if (ret <= 0 || !srmc->ini_cb.qp) {
                ret = ret ?: -EINVAL;
                WRITE_ONCE(sched->init_error, ret);
                wake_up_all(&sched->init_wait);
                break;
            }

            WRITE_ONCE(sched->srmc_by_idx[srmc->srmc_idx], srmc);
            smp_store_release(&sched->ready_srmc_cnt,
                              sched->ready_srmc_cnt + 1);
            wake_up_all(&sched->init_wait);
	            j = (j + 1) % NUM_SRMC;
	        }
	    }

	    if (has_srmc && flags == SRMC_CREATE_FLAG_INIT_QP &&
	        (smp_load_acquire(&sched->ready_srmc_cnt) < num_kqps ||
	         READ_ONCE(sched->init_error))) {
	        mutex_unlock(&sched->srmc_lock);
	        ret = wait_event_interruptible(sched->init_wait,
	            READ_ONCE(sched->init_error) ||
	            smp_load_acquire(&sched->ready_srmc_cnt) >= num_kqps);
	        if (ret)
	            return ret;
	        ret = READ_ONCE(sched->init_error);
	        if (ret)
	            return ret;
	        return 0;
	    }

	    mutex_unlock(&sched->srmc_lock);

    DEBUG_LOG("out is_xrc_exists,ret:%d\n", ret);
    return ret;
}

static void print_pd_info(struct ib_pd *pd)
{
    if (!pd)
    {
        pr_err("pd 指针为 NULL\n");
        return;
    }

    // 输出 local_dma_lkey 和 unsafe_global_rkey
    pr_info("SRMC pd->local_dma_lkey: %u\n", pd->local_dma_lkey);
    pr_info("SRMC pd->unsafe_global_rkey: %u\n", pd->unsafe_global_rkey);

    // 检查并输出 __internal_mr 的地址
    if (pd->__internal_mr)
    {
        pr_info("pd->__internal_mr 地址: %px\n", pd->__internal_mr);
        pr_info("pd->__internal_mr->lkey: %d\n", pd->__internal_mr->lkey);
        pr_info("pd->__internal_mr->rkey: %d\n", pd->__internal_mr->rkey);

        // 如果需要更多 MR 信息，可以继续添加
        pr_info("pd->__internal_mr->iova: 0x%llx\n", pd->__internal_mr->iova);
        pr_info("pd->__internal_mr->length: 0x%llx\n", pd->__internal_mr->length);
    }
    else
    {
        pr_info("SRMC pd->__internal_mr 为 NULL\n");
    }
}

// alloc dma buf and alloc mr, for ini qp
int mlx5_sched_alloc_mr(struct srm_cb *cb, struct ib_pd *pd)
{
    cb->buf_sz = 100;
    cb->buf = kzalloc(cb->buf_sz, GFP_KERNEL);
    if (cb->buf)
        cb->dma_buf = ib_dma_map_single(pd->device, cb->buf, cb->buf_sz, DMA_BIDIRECTIONAL);
    if (!cb->buf || ib_dma_mapping_error(pd->device, cb->dma_buf))
    {
        pr_err("Failed to allocate dma buffer\n");
        kfree(cb->buf);
        return -1;
    }

    dma_unmap_addr_set(cb, dma_mapping, cb->dma_buf);

    // alloc mr
    cb->page_list_len = (((cb->buf_sz - 1) & PAGE_MASK) + PAGE_SIZE) >> PAGE_SHIFT;
    cb->mr = ib_alloc_mr(pd, IB_MR_TYPE_MEM_REG, cb->page_list_len);
    if (IS_ERR(cb->mr))
    {
        pr_err("Failed to allocate mr\n");
        goto err;
    }
    return 0;
err:
    // TODO:error handling,free resources
    return -1;
}

// reg rkey and mr. for ini qp.
int mlx5_sched_reg_mr(struct ib_mr *mr, struct mlx5_ib_qp *qp, char *dma_buf, size_t buf_sz, int page_list_len)
{
    struct ib_reg_wr reg_wr = {0};
    struct ib_send_wr *bad_wr;
    int ret;
    struct scatterlist sg = {0};
    if (!mr || !qp)
    {
        pr_err("Unexpected:mr or qp is NULL\n");
        return -1;
    }
    reg_wr.wr.opcode = IB_WR_REG_MR;
    reg_wr.mr = mr;
    reg_wr.access = IB_ACCESS_LOCAL_WRITE | IB_ACCESS_REMOTE_WRITE;

    sg_init_marker(&sg, 1);

    ib_update_fast_reg_key(mr, 1);
    reg_wr.key = mr->rkey;

    sg_dma_address(&sg) = dma_buf;
    sg_dma_len(&sg) = buf_sz;

    ret = ib_map_mr_sg(mr, &sg, 1, NULL, PAGE_SIZE);
    BUG_ON(ret <= 0 || ret > page_list_len);

    ret = ib_post_send(&qp->ibqp, &reg_wr.wr, &bad_wr);
    if (ret)
    {
        pr_err("Failed to post reg mr\n");
    }
    return ret;
}


struct server_conn_info
{
    struct rdma_cm_id *cm_id;
    int flags;
};

int srm_create_connection(struct server_conn_info *conn_info)
{
    struct rdma_cm_id *cm_id = conn_info->cm_id;
    int flags = conn_info->flags;
    extern struct mlx5_ib_sched_group sched_group;
    struct mlx5_ib_srmc *srmc;
    struct mlx5_ib_sched *sched;
    union ib_gid dgid;
    struct ib_cq_init_attr cq_attr;
    struct ib_qp_init_attr init_attr;
    int idx;
    int no_srmc;
    struct srm_cb *cb, *server_cb;
    int ret;

    int i, hash_id, j;
    int found;
    int cnt;

    server_cb = (struct srm_cb *)cm_id->context;

    rdma_read_gids(cm_id, NULL, &dgid);
    DEBUG_LOG("in srm_create_connection,cma_id = %d,dgid.interface_id = %llx,dgid.subnet_prefix=%llx\n", cm_id, dgid.global.interface_id, dgid.global.subnet_prefix);

    idx = sched_hash_ip(dgid.raw + 12, sched_group.num_sched);

    DEBUG_LOG("idx=%d\n", idx);

    sched = &sched_group.scheds[idx];

    hash_id = sched_hash_ip(dgid.raw + 12, NUM_SRMC);
    found = 0;
    mutex_lock(&sched->srmc_lock);
    for (i = 0; i < NUM_SRMC; i++)
    {
        j = (hash_id + i) % NUM_SRMC;
        srmc = sched->srmc_tb[j];
        if (srmc == NULL)
        {
            break;
        }
        if (memcmp(srmc->dgid.raw, dgid.raw, sizeof(srmc->dgid.raw)) == 0)
        {
            found = 1;
            break;
        }
    }
    cnt = 0;
    while (srmc && srmc->tgt_cb.refcnt)
    {
        j = (j + 1) % NUM_SRMC;
        srmc = sched->srmc_tb[j];
        cnt++;
        if (cnt > NUM_SRMC)
        {
            pr_err("srmc queue is full\n");
            mutex_unlock(&sched->srmc_lock);
            rdma_destroy_id(cm_id);
            return -1;
        }
    }

    if (!srmc)
    {
        srmc = kzalloc(sizeof(struct mlx5_ib_srmc), GFP_KERNEL);
        sched->srmc_cnt++;
        memcpy(srmc->dgid.raw, dgid.raw, sizeof(srmc->dgid.raw));
        // 将srmc 加入到srmc_head中
        sched->srmc_tb[j] = srmc;
    }

    // srmc->tgt_cb.refcnt == 0
    srmc->tgt_cb.refcnt = 1;
    srmc->tgt_cb.cm_id = cm_id;
    srmc->tgt_cb.state = CONNECT_REQUEST;
    mutex_unlock(&sched->srmc_lock);
    cb = &srmc->tgt_cb;

    cb->txdepth = server_cb->txdepth;
    init_waitqueue_head(&cb->sem);
    cb->server = 1;

    // create pd
    cb->pd = ib_alloc_pd(cb->cm_id->device, 0);
    if (IS_ERR(cb->pd))
    {
        printk(KERN_ERR "alloc pd failed\n");
        ret = PTR_ERR(cb->pd);
        goto err0;
    }
    DEBUG_LOG("alloc pd\n");

    // create cq
    memset(&cq_attr, 0, sizeof cq_attr);
    cq_attr.cqe = cb->txdepth;
    cq_attr.comp_vector = 0;
    // change to event?
    cb->cq = ib_create_cq(cb->cm_id->device, NULL, NULL, NULL, &cq_attr);
    if (IS_ERR(cb->cq))
    {
        printk(KERN_ERR "ib_create_cq failed\n");
        ret = PTR_ERR(cb->cq);
        goto err1;
    }
    DEBUG_LOG("created cq %p,cqe:%d\n", cb->cq, cb->cq->cqe);

    // create qp
    memset(&init_attr, 0, sizeof(init_attr));
    // init_attr.cap.max_send_wr = cb->txdepth;
    init_attr.cap.max_recv_wr = cb->txdepth;

    /* For flush_qp() */
    // init_attr.cap.max_send_wr++;
    // init_attr.cap.max_recv_wr++;

    init_attr.cap.max_recv_sge = 1;
    // init_attr.cap.max_send_sge = 1;
    // init_attr.send_cq = cb->cq;
    init_attr.recv_cq = cb->cq;
    init_attr.sq_sig_type = IB_SIGNAL_REQ_WR;
    init_attr.qp_type = IB_QPT_XRC_TGT;

    cb->xrcd = ((struct srm_cb *)cm_id->context)->xrcd;
    init_attr.xrcd = cb->xrcd;
    DEBUG_LOG("xrcd = %p,txdepth = %d\n", init_attr.xrcd, init_attr.cap.max_recv_wr);
    if (cb->xrcd == NULL)
    {
        pr_err("xrcd is NULL\n");
        ret = -1;
        goto err2;
    }

    ret = rdma_create_qp(cb->cm_id, cb->pd, &init_attr);
    if (!ret)
        cb->qp = to_mqp(cb->cm_id->qp);
    else
    {
        pr_err("server rdma_create_qp failed,error:%d\n", ret);
        goto err2;
    }
    DEBUG_LOG("created qp %p\n", cb->qp);
    cm_id->context = (void *)&srmc->tgt_cb;

    // accept
    ret = srm_accept(cb);
    if (ret)
    {
        pr_err("accept failed\n");
        goto err3;
    }
    DEBUG_LOG("accept\n");

    DEBUG_LOG("srm_create_connection success\n");
    kfree(conn_info);
    return 0;

err3:
    ib_destroy_qp(&cb->qp->ibqp);
err2:
    ib_destroy_cq(cb->cq);
err1:
    ib_dealloc_pd(cb->pd);
err0:
    rdma_reject(cm_id, NULL, 0, IB_CM_REJ_CONSUMER_DEFINED);
    rdma_destroy_id(cm_id);
    kfree(conn_info);
    return ret;
}
static int srm_cma_event_handler(struct rdma_cm_id *cma_id,
                                 struct rdma_cm_event *event)
{
    int ret;
    int flags;
    struct server_conn_info *conn_info;
    struct srm_cb *cb = cma_id->context;

    printk("cma_event type %d cma_id %p (%s)\n", event->event, cma_id,
           (cma_id == cb->cm_id) ? "parent" : "child");

    switch (event->event)
    {
    case RDMA_CM_EVENT_ADDR_RESOLVED:
        cb->state = ADDR_RESOLVED;
        ret = rdma_resolve_route(cma_id, 2000);
        if (ret)
        {
            printk(KERN_ERR "rdma_resolve_route error %d\n",
                   ret);
            wake_up_interruptible(&cb->sem);
        }
        break;

    case RDMA_CM_EVENT_ROUTE_RESOLVED:
        cb->state = ROUTE_RESOLVED;
        wake_up_interruptible(&cb->sem);
        break;

    case RDMA_CM_EVENT_CONNECT_REQUEST:
        flags = *(int *)event->param.conn.private_data;
        DEBUG_LOG("connect request,flags = %d\n", flags);
        conn_info = kzalloc(sizeof(struct server_conn_info), GFP_KERNEL);
        conn_info->cm_id = cma_id;
        conn_info->flags = flags;
        kthread_run(srm_create_connection, conn_info, "server connection thread");
        printk("child cma %p\n", cma_id);
        break;

    case RDMA_CM_EVENT_ESTABLISHED:
        printk("ESTABLISHED\n");
        cb->state = CONNECTED;
        wake_up_interruptible(&cb->sem);
        break;

    case RDMA_CM_EVENT_ADDR_ERROR:
    case RDMA_CM_EVENT_ROUTE_ERROR:
    case RDMA_CM_EVENT_CONNECT_ERROR:
    case RDMA_CM_EVENT_UNREACHABLE:
    case RDMA_CM_EVENT_REJECTED:
        printk(KERN_ERR "cma event %d, error %d,reject msg:%s\n", event->event,
               event->status, rdma_reject_msg(cma_id, event->status));
        cb->state = ERROR;
        wake_up_interruptible(&cb->sem);
        break;

    case RDMA_CM_EVENT_DISCONNECTED:
        printk(KERN_ERR "DISCONNECT EVENT...\n");
        cb->state = ERROR;
        wake_up_interruptible(&cb->sem);
        break;

    case RDMA_CM_EVENT_DEVICE_REMOVAL:
        printk(KERN_ERR "cma detected device removal!!!!\n");
        cb->state = ERROR;
        wake_up_interruptible(&cb->sem);
        break;

    default:
        printk(KERN_ERR "oof bad type!\n");
        wake_up_interruptible(&cb->sem);
        break;
    }
    return 0;
}

static void fill_sockaddr(struct sockaddr_storage *sin, struct srm_cb *cb)
{
    memset(sin, 0, sizeof(*sin));

    struct sockaddr_in *sin4 = (struct sockaddr_in *)sin;
    sin4->sin_family = AF_INET;
    memcpy((void *)&sin4->sin_addr.s_addr, cb->addr, 4);
    sin4->sin_port = htons(cb->port);
}
static int reg_supported(struct ib_device *dev)
{
    u64 needed_flags = IB_DEVICE_MEM_MGT_EXTENSIONS;

    if ((dev->attrs.device_cap_flags & needed_flags) != needed_flags)
    {
        printk(KERN_ERR
               "Fastreg not supported - device_cap_flags 0x%llx\n",
               (unsigned long long)dev->attrs.device_cap_flags);
        return 0;
    }
    DEBUG_LOG("Fastreg supported - device_cap_flags 0x%llx\n",
              (unsigned long long)dev->attrs.device_cap_flags);
    return 1;
}

static int srm_bind_client(struct srm_cb *cb)
{
    struct sockaddr_storage sin;
    int ret;

    fill_sockaddr(&sin, cb);

    ret = rdma_resolve_addr(cb->cm_id, NULL, (struct sockaddr *)&sin, 2000);
    if (ret)
    {
        printk(KERN_ERR "rdma_resolve_addr error %d\n", ret);
        return ret;
    }

    wait_event_interruptible(cb->sem, cb->state >= ROUTE_RESOLVED);
    if (cb->state != ROUTE_RESOLVED)
    {
        printk(KERN_ERR
               "addr/route resolution did not resolve: state %d\n",
               cb->state);
        return -EINTR;
    }

    if (!reg_supported(cb->cm_id->device))
        return -EINVAL;

    DEBUG_LOG("rdma_resolve_addr - rdma_resolve_route successful\n");
    return 0;
}

static int srm_connect_client(struct srm_cb *cb, int flags)
{
    struct rdma_conn_param conn_param;
    int ret;

    memset(&conn_param, 0, sizeof conn_param);
    conn_param.responder_resources = 1;
    conn_param.initiator_depth = 1;
    conn_param.retry_count = 5;

    conn_param.private_data = &flags;
    conn_param.private_data_len = sizeof(int);

    ret = rdma_connect(cb->cm_id, &conn_param);
    if (ret)
    {
        printk(KERN_ERR "rdma_connect error %d\n", ret);
        return ret;
    }

    wait_event_interruptible(cb->sem, cb->state >= CONNECTED);
    if (cb->state == ERROR)
    {
        printk(KERN_ERR "wait for CONNECTED state %d\n", cb->state);
        return -1;
    }

    DEBUG_LOG("rdma_connect successful\n");
    return 0;
}

int create_srmc_qp_cm(struct mlx5_ib_srmc *srmc, struct ib_pd *pd, union ib_gid *dgid, int flags, int id, u32 sq_depth)
{
    struct srm_cb *cb;
    int ret;
    struct ib_cq_init_attr cq_attr;
    struct ib_qp_init_attr init_attr;

    cb = &srmc->ini_cb;

    cb->addr_str = IP_ADDR;
    cb->port = PORT_NUM;
    if (!(ret = in4_pton(cb->addr_str, -1, cb->addr, -1, NULL)))
    {
        printk(KERN_ERR "in4_pton error %d\n", ret);
        ret = -1;
        goto out;
    }
    DEBUG_LOG("Expected IPv4 address: %u.%u.%u.%u\n",
              cb->addr[0], cb->addr[1], cb->addr[2], cb->addr[3]);
    mlx5_ib_gid2ip(cb->addr, dgid);
    DEBUG_LOG("Real IPv4 address: %u.%u.%u.%u\n", cb->addr[0], cb->addr[1], cb->addr[2], cb->addr[3]);
    cb->server = 0;
    init_waitqueue_head(&cb->sem);
    cb->txdepth = sq_depth;
    cb->pd = pd;

    cb->cm_id = rdma_create_id(&init_net, srm_cma_event_handler, cb, RDMA_PS_TCP, IB_QPT_XRC_INI);
    if (IS_ERR(cb->cm_id))
    {
        ret = PTR_ERR(cb->cm_id);
        printk(KERN_ERR "rdma_create_id error %d\n", ret);
        goto out;
    }
    DEBUG_LOG("created cm_id %p\n", cb->cm_id);
    ret = srm_bind_client(cb);
    if (ret)
    {
        printk(KERN_ERR "bind client failed\n");
        goto err0;
    }

    // //create pd:now replace the userspace pd with kernel pd
    // cb->pd = ib_alloc_pd(cb->cm_id->device);
    // pd = cb->pd;
    // if (IS_ERR(cb->pd)){
    //     printk(KERN_ERR "ib_alloc_pd failed,pd:%s\n",PTR_ERR(cb->pd));
    //     ret = PTR_ERR(cb->pd);
    //     goto err0;
    // }

    // create cq
    memset(&cq_attr, 0, sizeof cq_attr);
    cq_attr.cqe = cb->txdepth;
    cq_attr.comp_vector = 0;
    // change to event?
    if (!shared_cq[id][srmc->srmc_idx % (CQ_NUM)])
        shared_cq[id][srmc->srmc_idx % (CQ_NUM)] = ib_create_cq(cb->cm_id->device, NULL, NULL, NULL, &cq_attr);
    cb->cq = shared_cq[id][srmc->srmc_idx % (CQ_NUM)];
    if (IS_ERR(cb->cq))
    {
        printk(KERN_ERR "ib_create_cq failed,cq:%s\n", PTR_ERR(cb->cq));
        ret = PTR_ERR(cb->cq);
        goto err0;
    }
    DEBUG_LOG("cq_num:%d\n", cb->cq->cqe);
    // ret = ib_req_notify_cq(cb->cq, IB_CQ_NEXT_COMP);
    // if (ret)
    // {
    //     printk(KERN_ERR "ib_req_notify_cq failed,cq:%s\n", PTR_ERR(cb->cq));
    //     goto err1;
    // }
    // create qp
    memset(&init_attr, 0, sizeof(init_attr));
    init_attr.cap.max_send_wr = cb->txdepth;
    init_attr.cap.max_recv_wr = cb->txdepth;

    /* For flush_qp() */
    // init_attr.cap.max_send_wr++;
    // init_attr.cap.max_recv_wr++;

    init_attr.cap.max_recv_sge = 1;
    init_attr.cap.max_send_sge = 1;
    init_attr.qp_type = IB_QPT_XRC_INI;
    init_attr.send_cq = cb->cq;
    init_attr.recv_cq = cb->cq;
    init_attr.sq_sig_type = IB_SIGNAL_REQ_WR;
    ret = rdma_create_qp(cb->cm_id, pd, &init_attr);
    if (!ret)
        cb->qp = to_mqp(cb->cm_id->qp);
    else
    {
        pr_err("rdma_create_qp failed,error:%d\n", ret);
        goto err1;
    }
    // pr_info("max_post:%d\n",cb->qp->sq.max_post);
    DEBUG_LOG("created qp %p\n", cb->qp);
    DEBUG_LOG("max_send_wr:%d\n", init_attr.cap.max_send_wr);

    // //alloc mr and buf
    // ret = mlx5_sched_alloc_mr(cb,pd);
    // if(ret){
    //     pr_err("alloc mr failed,error:%d\n",ret);
    //     goto err2;
    // }

    // modify qp etc
    ret = srm_connect_client(cb, flags);
    if (ret)
    {
        pr_err("connect client failed,error:%d\n", ret);
        goto err3;
    }

    

    return ret ? 0 : cb->qp->ibqp.qp_num;
err4:
    rdma_disconnect(cb->cm_id);
err3:
    // TODO:free buf and mr resources.
    // ib_sched_free_buf(cb);
err2:
    ib_destroy_qp(&cb->qp->ibqp);
err1:
    ib_destroy_cq(cb->cq);
err0:
    rdma_destroy_id(cb->cm_id);
    cb->cm_id = NULL;
out:
    return ret ? 0 : cb->qp->ibqp.qp_num;
}

int srm_bind_server(struct srm_cb *cb)
{
    struct sockaddr_storage sin;
    int ret;

    fill_sockaddr(&sin, cb);

    ret = rdma_bind_addr(cb->cm_id, (struct sockaddr *)&sin);
    if (ret)
    {
        printk(KERN_ERR "rdma_bind_addr error %d\n", ret);
        return ret;
    }
    DEBUG_LOG("rdma_bind_addr successful\n");

    return 0;
}
int srm_accept(struct srm_cb *cb)
{
    struct rdma_conn_param conn_param;
    int ret;

    DEBUG_LOG("accepting client connection request\n");

    memset(&conn_param, 0, sizeof conn_param);
    conn_param.responder_resources = 1;
    conn_param.initiator_depth = 1;

    ret = rdma_accept(cb->cm_id, &conn_param);
    if (ret)
    {
        printk(KERN_ERR "rdma_accept error: %d\n", ret);
        return ret;
    }

    wait_event_interruptible(cb->sem, cb->state >= CONNECTED);
    if (cb->state == ERROR)
    {
        printk(KERN_ERR "wait for CONNECTED state %d\n",
               cb->state);
        return -1;
    }
    return 0;
}
int mlx5_sched_run_server(struct srm_cb *cb)
{
    pr_info("srm server is running\n");
    int ret;
    struct ib_cq_init_attr cq_attr;
    struct ib_qp_init_attr init_attr;
    struct ib_pd *pd;

    cb->addr_str = IP_ADDR;
    cb->port = PORT_NUM;
    if (!(ret = in4_pton(cb->addr_str, -1, cb->addr, -1, NULL)))
    {
        printk(KERN_ERR "in4_pton error %d\n", ret);
        ret = -1;
        goto out;
    }
    cb->server = 1;
    init_waitqueue_head(&cb->sem);
    cb->txdepth = SQ_DEPTH;

    cb->cm_id = rdma_create_id(&init_net, srm_cma_event_handler, cb, RDMA_PS_TCP, IB_QPT_XRC_TGT);
    if (IS_ERR(cb->cm_id))
    {
        ret = PTR_ERR(cb->cm_id);
        printk(KERN_ERR "rdma_create_id error %d\n", ret);
        goto out;
    }
    DEBUG_LOG("created cm_id %p\n", cb->cm_id);

    ret = srm_bind_server(cb);
    if (ret)
    {
        printk(KERN_ERR "bind server failed\n");
        goto err0;
    }
    DEBUG_LOG("bind server\n");

    printk("rdma_listen\n");
    ret = rdma_listen(cb->cm_id, 10);
    if (ret)
    {
        printk(KERN_ERR "rdma_listen failed: %d\n", ret);
        goto err0;
    }

    wait_event_interruptible(cb->sem, kthread_should_stop());
    pr_info("srm server stop\n");
    return ret;
err0:
    rdma_destroy_id(cb->cm_id);
    cb->cm_id = NULL;
out:
    wait_event_interruptible(cb->sem, kthread_should_stop());
    return ret;
}

void ib_sched_free_buf(struct srm_cb *cb)
{
    DEBUG_LOG("ib_sched_free_buf\n");
    ib_dereg_mr(cb->mr);
    dma_unmap_single(cb->pd->device->dma_device, dma_unmap_addr(cb, dma_mapping), cb->buf_sz, DMA_BIDIRECTIONAL);
    kfree(cb->buf);
}
int sched_hash_ip(char addr[4], int n)
{
    // DEBUG_LOG("in sched_hash_ip\n");
    u32 hash = jhash(addr, 4, 0);
    return hash % n;
}

int mlx5_ib_register_external_table(void *table, size_t size, struct page **pages, void *level_table, size_t level_size, struct page **level_pages,
                                           void *xrc_table, size_t xrc_size, struct page **xrc_pages, int xrc_qp_num_per_srm)
{
    int prev_num_user_threads = num_user_threads;
    user_wqe_table = (struct aligned_u32*)table;
    user_wqe_pages = pages;
    if (size % sizeof(struct aligned_u32) != 0)
    {
        pr_err("error:size can't be divided,size:%d\n", size);
    }
    num_table_qp = size / sizeof(struct aligned_u32);
    pr_info("qp数量：%d\n", num_table_qp);

    user_level_table = (struct aligned_u32 *)level_table;
    user_level_pages = level_pages;
    if (level_size % sizeof(struct aligned_u32) != 0)
    {
        pr_err("error:level_size can't be divided,level_size:%d\n", level_size);
    }
    num_table_level = level_size / sizeof(struct aligned_u32);
    pr_info("level数量：%d\n", num_table_level);

    num_table_apps = num_table_level / (NUM_LEVEL * NUM_SCHED);
    if (num_table_apps <= 0)
        num_table_apps = 1;

    int i, j;

    num_user_threads = num_table_qp / (NUM_LEVEL*NUM_SCHED);

    if (num_user_threads > MAX_USER_THREADS_NUM) {
        pr_err("num_user_threads(%d) exceeds MAX_USER_THREADS_NUM(%d)\n",
               num_user_threads, MAX_USER_THREADS_NUM);
        return -EINVAL;
    }

    if (user_xrc_table) {
        for (i = 0; i < prev_num_user_threads; i++) {
            if (user_xrc_table[i])
                kfree(user_xrc_table[i]);
        }
        kfree(user_xrc_table);
        user_xrc_table = NULL;
    }

    user_xrc_table = kmalloc(sizeof(struct xrc_table_entry**) * num_user_threads, GFP_KERNEL);
    if (!user_xrc_table)
        return -ENOMEM;
    for(i = 0;i<num_user_threads;i++){
        user_xrc_table[i] = kmalloc(sizeof(struct xrc_table_entry*) * (NUM_LEVEL*NUM_SCHED), GFP_KERNEL);
        if (!user_xrc_table[i]) {
            while (--i >= 0)
                kfree(user_xrc_table[i]);
            kfree(user_xrc_table);
            user_xrc_table = NULL;
            return -ENOMEM;
        }
        for(j=0;j<NUM_LEVEL*NUM_SCHED;j++){
            user_xrc_table[i][j] = (struct xrc_table_entry *)xrc_table + i*(NUM_LEVEL*NUM_SCHED)*MAX_USER_XRC_QP_PER_SRM + j*MAX_USER_XRC_QP_PER_SRM;
        }
    }

    user_xrc_pages = xrc_pages;

    num_xrc_per_srm = xrc_qp_num_per_srm;

    num_xrc_qp = num_user_threads * NUM_SCHED * num_xrc_per_srm;

    pr_info("num_xrc_per_srm:%d,num_xrc_qp:%d,num_user_threads:%d\n",num_xrc_per_srm,num_xrc_qp,num_user_threads);

    /* Clear stale accounting carried across previous runs/sessions. */
    memset(kernel_wqe_table, 0, sizeof(kernel_wqe_table));
    memset(kernel_level_table, 0, sizeof(kernel_level_table));
    memset(tot_xrc_sended_wqes, 0, sizeof(tot_xrc_sended_wqes));
    memset(cur_xrc_sended_wqes, 0, sizeof(cur_xrc_sended_wqes));
    memset(lst_xrc_bytes, 0, sizeof(lst_xrc_bytes));

    


    
    
   
    return 0;
}
EXPORT_SYMBOL_GPL(mlx5_ib_register_external_table);
