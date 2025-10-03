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

// 文件操作
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "mlx5_avl_tree.h"

#define IP_ADDR "192.168.1.5"
#define PORT_NUM 12345
#define SRMC_POLLING_CNT 10000
#define WQES_ARR_SZ 16
#define SCHED_NUM 2

// 2. 位运算替代取模（需确保CQ_NUM是2的幂，如16、32）
#define CQ_NUM_POWER 0 // 示例：CQ_NUM=2^4=16
#define CQ_NUM (1 << CQ_NUM_POWER)
#define CQ_MOD(srmc_idx) ((srmc_idx) & (CQ_NUM - 1)) // 位运算替代取模

// 3. 提前计算索引宏（减少循环内重复计算）
#define LEVEL_TABLE_IDX(level, id) ((level) + (4) * (id))       // level_table索引
#define NUM_THREAD_QPS_PER_SCHED(num_sched) ((4) * (num_sched)) // 每个调度器的线程QP数
#define CALC_N(k, num_thread_qps_per_sched, level, num_user_threads, id_per_thread_qp_nums) \
    ((k) / (num_thread_qps_per_sched) + (level) * (num_user_threads) + (id_per_thread_qp_nums))

const size_t MESSAGE_SIZE_THRESHOLD = 1024 * 10;
// const size_t MESSAGE_SIZE_THRESHOLD = 1e9;
const size_t QUEUE_LIMIT = 256 * 1024;
const size_t SCHED_SIZE_LIMIT = 8 * 1024;

static uint32_t *user_wqe_table, *user_level_table, *user_idx_table; // 用户态mmap表，表示当前多少个wqe已经下发
static struct page **user_wqe_pages, **user_level_pages, **user_idx_pages;
uint32_t kernel_wqe_table[NUM_SQB], kernel_level_table[4 * SCHED_NUM]; // 内核态表，表示当前srm qp中内核已发送多少个wqe
int num_table_qp, num_table_level;
struct ib_cq *shared_cq[SCHED_NUM][CQ_NUM]; // 每个内核线程一个cq,大小srmc各CQ_NUM个cq

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
    uq->wqe_cnt = size / MLX5_SEND_WQE_BB;
    DEBUG_LOG("sqb->wqe_cnt:%d\n", uq->wqe_cnt);
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

    mutex_lock(&sched_group->cq_lock);
    for (i = 0; i < sched_group->cqb_cnt; i++)
    {
        cqb = sched_group->cqb_arr[i];
        if (cqb->cqn == cqn)
        {
            uq->cqb = cqb;
            break;
        }
    }
    if (uq->cqb == NULL)
    {
        pr_err("cqn %d not found\n", cqn);
    }
    mutex_unlock(&sched_group->cq_lock);

    sched_group->sqb_arr[sched_group->sqb_cnt] = uq;
    uq->idx = sched_group->sqb_cnt; // for debug only
    sched_group->sqb_cnt++;

    // pr_info("map sq buf %d success\n", sched_group->sqb_cnt - 1);
    mutex_unlock(&sched_group->sq_lock);

    return 0;
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

int mlx5_ib_unmap_ubuf(struct mlx5_ib_sched_group *sched_group, int qpn)
{
    struct mlx5_ib_sqbuf *sqb;
    struct mlx5_ib_cqbuf *cqb;
    int cqn;
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
            cqn = sqb->cqb->cqn;
            vunmap(sqb->buf);
            npages = (sqb->sq_size + PAGE_SIZE - 1) / PAGE_SIZE;
            for (i = 0; i < npages; i++)
                put_page(sqb->pages[i]);
            kfree(sqb->pages);
            kfree(sqb);

            break;
        }
    }

    mutex_unlock(&sched_group->sq_lock);

    // Free cq
    mutex_lock(&sched_group->cq_lock);

    for (i = 0; i < sched_group->cqb_cnt; i++)
    {
        cqb = sched_group->cqb_arr[i];

        if (cqb == NULL)
            continue;
        if (cqb->cqn == cqn)
        {
            sched_group->cqb_arr[i] = NULL;
            vunmap(cqb->buf);
            npages = (cqb->cq_size + PAGE_SIZE - 1) / PAGE_SIZE;
            for (i = 0; i < npages; i++)
                put_page(cqb->pages[i]);
            kfree(cqb->pages);
            kfree(cqb);

            break;
        }
    }
    mutex_unlock(&sched_group->cq_lock);
    return 0;
}
// static void srm_cq_event_handler(struct ib_cq *cq, void *ctx)
// {
//     struct mlx5_ib_srmc *srmc;
//     struct ib_wc wc;
//     void *cqe, *ucqe;
//     int qpn;
//     struct mlx5_ib_sqbuf *sqb;
//     struct mlx5_ib_cqbuf *cqb;
//     struct mlx5_cqe64 *ucqe64;
//     int idx;
//     srmc = ctx;

//     DEBUG_LOG("distributing cqe\n");
//     // memset(&wc, 1, sizeof wc);
//     int cqe_num = 0;
//     while ((cqe_num = mlx5_ib_poll_cq_with_cqe(srmc->ini_cb.qp->ibqp.send_cq, 1, &wc, &cqe)) == 1)
//     {
//         // cqe64 = (to_mcq(srmc->ini_cb.qp->ibqp.send_cq)->mcq.cqe_sz == 64) ? cqe : cqe + 64;
//         //  Two attr to change
//         idx = wc.wr_id;
//         // qpn = (wc.wr_id >> 32) & 0xffffff; // wr_id high 32 bits is qpn,low  32 bits is wqe_counter
//         qpn = srmc->wqe_infos[idx].qpn;
//         DEBUG_LOG("cqe_num:%d,wc status:%d,byte_cnt:%d\n", cqe_num, wc.status, srmc->wqe_infos[idx].pending_bytes);
//         DEBUG_LOG("wc's qpn:%d,wc's wqe_counter:%d\n", qpn, srmc->wqe_infos[idx].wqe_counter);
//         sqb = srmc->wqe_infos[idx].sqb;
//         if (sqb == NULL || sqb->cqb == NULL)
//         {
//             pr_err("Unexpected:No cqn found for qpn %d\n", qpn);
//         }
//         cqb = sqb->cqb;
//         mutex_lock(&cqb->lock); // 多个线程可能同时写入同一个cq
//         // distribute
//         // TODO:change the owner bit
//         DEBUG_LOG("cqn:%d\n", cqb->cqn);
//         ucqe = cqb->buf + cqb->cur_put * cqb->cqe_sz;
//         ucqe64 = (cqb->cqe_sz == 64) ? ucqe : ucqe + 64;
//         memcpy(ucqe, cqe, cqb->cqe_sz);
//         DEBUG_LOG("cqe64->op_own:%x,cqe_size:%d\n", ucqe64->op_own, cqb->cqe_sz);
//         ucqe64->sop_drop_qpn = htonl(ntohl(ucqe64->sop_drop_qpn) & (~0xffffff) | qpn);
//         ucqe64->wqe_counter = htons(srmc->wqe_infos[idx].wqe_counter & 0xffff);
//         // 反转用户态cqe的owner_bit
//         ucqe64->op_own = (ucqe64->op_own & (~0xf)) | cqb->op_own;
//         // 根据cqe v1，保存uidx
//         ucqe64->srqn = htonl(sqb->uidx);
//         // 减去发送中的字节数
//         srmc->pending_bytes -= srmc->wqe_infos[idx].pending_bytes;
//         DEBUG_LOG("ucqe64->op_own:%x,op_own:%d\n", ucqe64->op_own, cqb->op_own);
//         cqb->cur_put++;
//         if (cqb->cur_put * cqb->cqe_sz >= cqb->cq_size)
//         {
//             cqb->cur_put = 0;
//             cqb->op_own ^= MLX5_CQE_OWNER_MASK;
//         }

//         mutex_unlock(&cqb->lock);
//     }
//     DEBUG_LOG("distribute cqe finished\n");
//     ib_req_notify_cq(srmc->ini_cb.qp->ibqp.send_cq, IB_CQ_NEXT_COMP);
// }

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
        if (cnt_c >= sched->srmc_cnt[0])
        {
            break;
        }
        srmc = sched->srmc_small_tb[i];
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
                    idx = wc[j].wr_id & (SQ_DEPTH - 1); // cq大小为SQ_DEPTH
                    // 减去发送中的字节数
                    srmc->pending_bytes -= srmc->wqe_infos[idx].pending_bytes;
                    if (srmc->wqe_infos[idx].to_user == 0)
                    {
                        continue;
                    }
                    // qpn = (wc.wr_id >> 32) & 0xffffff; // wr_id high 32 bits is qpn,low  32 bits is wqe_counter
                    qpn = srmc->wqe_infos[idx].qpn;
                    DEBUG_LOG("cqe_num:%d,wc status:%d,byte_cnt:%d\n", cqe_num, wc[j].status, srmc->wqe_infos[idx].pending_bytes);
                    DEBUG_LOG("wc's qpn:%d,wc's wqe_counter:%d\n", qpn, srmc->wqe_infos[idx].wqe_counter);
                    sqb = srmc->wqe_infos[idx].sqb;
                    if (sqb == NULL || sqb->cqb == NULL)
                    {
                        pr_err("Unexpected:No cqn found for qpn %d\n", qpn);
                    }
                    cqb = sqb->cqb;
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
                    ucqe64->srqn = htonl(sqb->uidx);

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

    // large cqes
    cnt_c = 0;
    for (i = 0; i < NUM_SRMC; i++)
    {
        if (cnt_c >= sched->srmc_cnt[1])
        {
            break;
        }
        srmc = sched->srmc_large_tb[i];
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
                    idx = wc[j].wr_id & (SQ_DEPTH - 1); // cq大小为SQ_DEPTH
                    // 减去发送中的字节数
                    srmc->pending_bytes -= srmc->wqe_infos[idx].pending_bytes;
                    if (srmc->wqe_infos[idx].to_user == 0)
                    {
                        continue;
                    }
                    // qpn = (wc.wr_id >> 32) & 0xffffff; // wr_id high 32 bits is qpn,low  32 bits is wqe_counter
                    qpn = srmc->wqe_infos[idx].qpn;
                    DEBUG_LOG("cqe_num:%d,wc status:%d,byte_cnt:%d\n", cqe_num, wc[j].status, srmc->wqe_infos[idx].pending_bytes);
                    DEBUG_LOG("wc's qpn:%d,wc's wqe_counter:%d\n", qpn, srmc->wqe_infos[idx].wqe_counter);
                    sqb = srmc->wqe_infos[idx].sqb;
                    if (sqb == NULL || sqb->cqb == NULL)
                    {
                        pr_err("Unexpected:No cqn found for qpn %d\n", qpn);
                    }
                    cqb = sqb->cqb;
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
                    ucqe64->srqn = htonl(sqb->uidx);

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
                idx = wc[i].wr_id & (SQ_DEPTH - 1); // cq大小为SQ_DEPTH
                // 减去发送中的字节数
                srmc->pending_bytes -= srmc->wqe_infos[idx].pending_bytes;
                if (srmc->wqe_infos[idx].to_user == 0)
                {
                    continue;
                }
                // qpn = (wc.wr_id >> 32) & 0xffffff; // wr_id high 32 bits is qpn,low  32 bits is wqe_counter
                qpn = srmc->wqe_infos[idx].qpn;
                DEBUG_LOG("cqe_num:%d,wc status:%d,byte_cnt:%d\n", cqe_num, wc[i].status, srmc->wqe_infos[idx].pending_bytes);
                DEBUG_LOG("wc's qpn:%d,wc's wqe_counter:%d\n", qpn, srmc->wqe_infos[idx].wqe_counter);
                sqb = srmc->wqe_infos[idx].sqb;
                if (sqb == NULL || sqb->cqb == NULL)
                {
                    pr_err("Unexpected:No cqn found for qpn %d\n", qpn);
                }
                cqb = sqb->cqb;
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
                ucqe64->srqn = htonl(sqb->uidx);

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
                idx = wc[i].wr_id & (SQ_DEPTH - 1); // cq大小为SQ_DEPTH
                // 减去发送中的字节数
                srmc->pending_bytes -= srmc->wqe_infos[idx].pending_bytes;
                if (srmc->wqe_infos[idx].to_user == 0)
                {
                    continue;
                }
                // qpn = (wc.wr_id >> 32) & 0xffffff; // wr_id high 32 bits is qpn,low  32 bits is wqe_counter
                qpn = srmc->wqe_infos[idx].qpn;
                DEBUG_LOG("cqe_num:%d,wc status:%d,byte_cnt:%d\n", cqe_num, wc[i].status, srmc->wqe_infos[idx].pending_bytes);
                DEBUG_LOG("wc's qpn:%d,wc's wqe_counter:%d\n", qpn, srmc->wqe_infos[idx].wqe_counter);
                sqb = srmc->wqe_infos[idx].sqb;
                if (sqb == NULL || sqb->cqb == NULL)
                {
                    pr_err("Unexpected:No cqn found for qpn %d\n", qpn);
                }
                cqb = sqb->cqb;
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
                ucqe64->srqn = htonl(sqb->uidx);

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
                len = scnprintf(buf, 256, "polled user cqe,k:%d,byte_cnt:%d,elapsed time from last poll:%llu(ns)\n", sqb->idx, srmc->wqe_infos[idx].byte_cnt, elapsed_ns);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
                /* kernel_write 从 5.11+ 内核可用，无需 set_fs */
                ret = kernel_write(filp, buf, len, pos);
#else
                ret = vfs_write(filp, buf, len, pos);
#endif
                if (ret < 0)
                    pr_err("write_int_to_file: write error %d\n", ret);

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
        ret += user_wqe_table[i] - kernel_wqe_table[i];
    }
    return ret;
}

const int num_kqps = 512;
// -------------------------- 全局常量与宏定义（优化1：预处理优化） --------------------------
// 1. 全局Polling顺序表（避免栈上重复初始化）
static const int polling_order[4][4] = {
    {0, 1, 2, 3},
    {1, 0, 2, 3},
    {2, 3, 1, 0},
    {3, 2, 1, 0}};

// -------------------------- 内联函数（优化3：减少冗余代码） --------------------------
/**
 * poll_srmc_queue - 处理pre_srmcs队列的CQ Polling（提取重复逻辑）
 * @pre_srmcs: SRMC Polling队列
 * @polling_tail: 队列尾指针
 * @polling_head: 队列头指针
 * @in_queue: SRMC入队标记
 * @wc: IB完成队列结构体
 * @cqe: CQE缓冲区
 * @free_cqe_idx: 空闲CQE索引列表
 * @free_cqe_cnt: 空闲CQE计数
 */
static __always_inline void poll_srmc_queue(struct mlx5_ib_srmc **pre_srmcs, int *polling_tail, int *polling_head,
                                            u8 *in_queue, struct ib_wc *wc, void **cqe,
                                            int *free_cqe_idx, int *free_cqe_cnt, int id)
{
    struct mlx5_ib_srmc *pre_srmc = pre_srmcs[*polling_tail];
    if (!pre_srmc)
        return;

    // 出队
    pre_srmcs[*polling_tail] = NULL;
    (*polling_tail) = ((*polling_tail) + 1) % SRMC_POLLING_CNT;

    // Poll CQE
    int ret = srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, free_cqe_cnt);
    if (!pre_srmc->sig_cnt)
    {
        in_queue[CQ_MOD(pre_srmc->srmc_idx)] = 0;
        DEBUG_LOG("poll srmc[%d] done, sig_cnt=0", pre_srmc->srmc_idx);
        return;
    }

    // 队列满：强制Poll完
    if (pre_srmcs[*polling_head] != NULL)
    {
        pr_err_once("[id=%d] cq polling queue overflow\n", id);
        while (pre_srmc->sig_cnt)
        {
            srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, free_cqe_cnt);
        }
        in_queue[CQ_MOD(pre_srmc->srmc_idx)] = 0;
        return;
    }

    // SQ满或CQE满：强制Poll到满足条件
    struct mlx5_ib_qp *qp = pre_srmc->ini_cb.qp;
    if (!qp)
    {
        pr_err_once("[id=%d] srmc[%d] qp is NULL", id, pre_srmc->srmc_idx);
        in_queue[CQ_MOD(pre_srmc->srmc_idx)] = 0;
        return;
    }

    if (pre_srmc->sig_cnt >= SQ_DEPTH || (int)(qp->sq.head - qp->sq.tail) >= qp->sq.max_post)
    {
        if (pre_srmc->sig_cnt >= SQ_DEPTH)
            pr_err_once("[id=%d] srmc[%d] cqe exceed SQ_DEPTH", id, pre_srmc->srmc_idx);
        else
            pr_err_once("[id=%d] srmc[%d] sq exceed max_post (head=%d, tail=%d, max=%d)",
                        id, pre_srmc->srmc_idx, qp->sq.head, qp->sq.tail, qp->sq.max_post);

        while (pre_srmc->sig_cnt >= SQ_DEPTH || (int)(qp->sq.head - qp->sq.tail) >= qp->sq.max_post)
        {
            srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, free_cqe_cnt);
        }

        if (!pre_srmc->sig_cnt)
        {
            in_queue[CQ_MOD(pre_srmc->srmc_idx)] = 0;
            return;
        }
    }

    // 重新入队
    pre_srmcs[*polling_head] = pre_srmc;
    (*polling_head) = ((*polling_head) + 1) % SRMC_POLLING_CNT;
    DEBUG_LOG("poll srmc[%d] incomplete, requeue (sig_cnt=%d)", pre_srmc->srmc_idx, pre_srmc->sig_cnt);
}

/**
 * find_target_srmc - 查找目标SRMC（提取重复逻辑）
 * @sched: 调度器结构体
 * @gid: 目标GID
 * @length: WQE长度
 * @hash_id: 哈希ID（输入/输出）
 * @srm_seed: 随机种子
 * @id: 调度器ID
 * 返回：目标SRMC指针（NULL表示失败）
 */
static __always_inline struct mlx5_ib_srmc *find_target_srmc(struct mlx5_ib_sched *sched, const union ib_gid *gid,
                                                             uint32_t length, uint32_t *hash_id, uint64_t *srm_seed,
                                                             int id)
{
    int is_large_msg = (length > MESSAGE_SIZE_THRESHOLD);
    struct mlx5_ib_srmc *srmc = NULL;
    int i;

    // 哈希查找匹配GID的SRMC
    for (i = 0; i < NUM_SRMC; i++)
    {
        int j = (i + (*hash_id)) % NUM_SRMC;
        srmc = is_large_msg ? sched->srmc_large_tb[j] : sched->srmc_small_tb[j];
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
            srmc = is_large_msg ? sched->srmc_large_tb[j] : sched->srmc_small_tb[j];
            return srmc;
        }
    }

    pr_err_once("[id=%d] no srmc found for gid (length=%d)", id, length);
    return NULL;
}

// -------------------------- 主调度函数（核心优化） --------------------------
int scheduler_polling(void *sched_data)
{
    extern struct mlx5_ib_sched_group sched_group;
    int ret = 0;
    struct mlx5_ib_sched_id *sched_id = (struct mlx5_ib_sched_id *)sched_data;
    struct mlx5_ib_sched *sched = sched_id->sched;
    const int id = sched_id->id; // 常量化，避免重复访问
    struct ib_wc *wc = NULL;
    void **cqe = NULL;
    union ib_gid gid;
    unsigned long flags;
    u32 mlx5_opcode, opmod;
    u8 next_fence, fence, to_user, sig;
    int i;
    const int wqes_limit_sz = 62 * 1024; // 124KB
    int l;
    int m;
    struct mlx5_ib_srmc *pre_srmc, *srmc;
    struct mlx5_ib_srmc *cq_srmc_tb[CQ_NUM] = {0};
    uint64_t cur_wqes[WQES_ARR_SZ] = {0}; 
    int rd;
    int found;
    int j;

    // -------------------------- 1. 内存分配（优化4：动态分配大数组） --------------------------
    cqe = kmalloc_array(SQ_DEPTH, sizeof(void *), GFP_KERNEL);
    wc = kmalloc_array(SQ_DEPTH, sizeof(struct ib_wc), GFP_KERNEL);
    struct mlx5_ib_srmc **pre_srmcs = kmalloc_array(SRMC_POLLING_CNT, sizeof(*pre_srmcs), GFP_KERNEL);
    u8 *in_queue = kmalloc_array(CQ_NUM, sizeof(u8), GFP_KERNEL); // 用CQ_NUM替代NUM_SRMC*2，减少内存
    int *free_cqe_idx = kmalloc_array(SQ_DEPTH, sizeof(int), GFP_KERNEL);
    int *level_qp_st_arr = kmalloc_array(4, sizeof(int), GFP_KERNEL);


    // 内存分配检查（内核规范：必须检查NULL）
    if (!cqe || !wc || !pre_srmcs || !in_queue || !free_cqe_idx || !level_qp_st_arr || !cur_wqes)
    {
        pr_err("[id=%d] kmalloc failed", id);
        ret = -ENOMEM;
        goto err_mem;
    }

    // 初始化（批量初始化，减少循环次数）
    memset(&gid, 0, sizeof(gid));
    memset(gid.raw + 10, 0xff, 2); // GID格式初始化
    memset(pre_srmcs, 0, SRMC_POLLING_CNT * sizeof(*pre_srmcs));
    memset(in_queue, 0, CQ_NUM * sizeof(u8));
    memset(level_qp_st_arr, 0, 4 * sizeof(int));
    for (i = 0; i < WQES_ARR_SZ; i++)
    {
        cur_wqes[i] = 4096;
    }
    for (i = 0; i < SQ_DEPTH; i++)
    {
        free_cqe_idx[i] = i;
    }

    // -------------------------- 2. 常量预计算（优化1：减少循环内计算） --------------------------
    int free_cqe_cnt = SQ_DEPTH;
    int polling_tail = 0, polling_head = 0;
    int wqe_cur_idx = 0;
    uint64_t wqe_tot_sz = 4096 * WQES_ARR_SZ;
    uint64_t srm_seed = 0xdeadbeef;
    uint64_t polling_seed = 0xdeadbeef;
    const int num_sched = sched_group.num_sched;
    const int num_thread_qps_per_sched = NUM_THREAD_QPS_PER_SCHED(num_sched); // 预计算：4*num_sched
    int num_user_threads = 0, num_thread_qps = 0, per_thread_qp_nums = 0;
    int id_per_thread_qp_nums = 0; // 预计算：id * per_thread_qp_nums

    // -------------------------- 3. 初始化等待（优化5：动态睡眠） --------------------------
    while (!kthread_should_stop())
    {
        if (sched_group.sqb_cnt == 0 || num_table_qp != sched_group.sqb_cnt)
        {
            msleep(10); // 未就绪时延长睡眠，减少无效轮询
            continue;
        }

        // 预计算线程相关常量（仅初始化一次）
        num_user_threads = num_table_qp / (4 * num_sched);
        num_thread_qps = 4 * num_sched;
        per_thread_qp_nums = sched_group.sqb_cnt / num_sched;
        id_per_thread_qp_nums = id * per_thread_qp_nums; // 预计算索引
        DEBUG_LOG("init done (num_user_threads=%d, per_thread_qp_nums=%d)",
                  num_user_threads, per_thread_qp_nums);
        break;
    }

    // -------------------------- 4. 主轮询循环（核心优化） --------------------------
    uint64_t cnt = 0;
    while (!kthread_should_stop())
    {
        const uint64_t target_sz = wqes_limit_sz - (wqe_tot_sz - cur_wqes[wqe_cur_idx]);
        int order_idx;
        if (target_sz <= 0)
        {
            order_idx = 0;
        }
        else if (target_sz <= 4096)
        {
            order_idx = 1;
        }
        else if (target_sz <= 10240)
        {
            order_idx = 1;
        }
        else if (target_sz <= 102400)
        {
            // 这里开始取同等级的
            order_idx = 2;
        }
        else
        {
            order_idx = 3;
        }
        int send_ok = 0;
        u8 tfree = 1;

        // 遍历4个等级（优化6：提前缓存level_table索引）
        for (l = 0; l < 4; l++)
        {
            const int level = polling_order[order_idx][l];
            const int level_table_idx = LEVEL_TABLE_IDX(level, id); // 预计算索引
            const int user_level_val = smp_load_acquire(&user_level_table[level_table_idx]);
            const int level_wqe_cnt = user_level_val - kernel_level_table[level_table_idx];

            // 1. Poll SRMC队列（优化3：调用内联函数）
            // 每等级遍历时，先poll cqe
            pre_srmc = pre_srmcs[polling_tail];
            pre_srmcs[polling_tail] = NULL;
            if (pre_srmc)
            {
                polling_tail = (polling_tail + 1) % SRMC_POLLING_CNT;
                // ret = srm_poll_srmc_once_debug(pre_srmc, wc, cqe, filp, &pos, buf, &start_cycles_cq, &end_cycles_cq,free_cqe_idx,&free_cqe_cnt); // 文件
                ret = srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, &free_cqe_cnt);
                if (pre_srmc->sig_cnt)
                {
                    // 这次没poll完
                    if (unlikely(pre_srmcs[polling_head] != NULL))
                    {
                        pr_info("cq polling queue exceed queue length\n");
                        // 此时polling队列满，必须poll完
                        while (pre_srmc->sig_cnt)
                        {
                            srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, &free_cqe_cnt);
                        }
                        in_queue[pre_srmc->srmc_idx % (CQ_NUM)] = 0;
                    }
                    else if (unlikely(pre_srmc->sig_cnt >= SQ_DEPTH || (int)(pre_srmc->ini_cb.qp->sq.head - pre_srmc->ini_cb.qp->sq.tail) >= pre_srmc->ini_cb.qp->sq.max_post))
                    {
                        if (pre_srmc->sig_cnt >= SQ_DEPTH)
                            pr_info("cq queue exceed SQ_DEPTH\n");
                        else
                        {
                            pr_info("exceed max_post,sq.head:%d, sq.tail:%d,max_post:%d",
                                    pre_srmc->ini_cb.qp->sq.head, pre_srmc->ini_cb.qp->sq.tail, pre_srmc->ini_cb.qp->sq.max_post);
                        }
                        // cqe队列满或者sq队列满，必须poll到一个以上,让sig_cnt小于SQ_DEPTH
                        while (pre_srmc->sig_cnt >= SQ_DEPTH || pre_srmc->ini_cb.qp->sq.head - pre_srmc->ini_cb.qp->sq.tail >= pre_srmc->ini_cb.qp->sq.max_post)
                        {
                            srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, &free_cqe_cnt);
                        }
                        if (!pre_srmc->sig_cnt)
                        {
                            // poll完
                            in_queue[pre_srmc->srmc_idx % (CQ_NUM)] = 0;
                        }
                        else
                        {
                            // 没poll完，重新加入队列
                            pre_srmcs[polling_head] = pre_srmc;
                            polling_head = (polling_head + 1) % SRMC_POLLING_CNT;
                        }
                    }
                    else
                    {
                        // 重新加入队列
                        pre_srmcs[polling_head] = pre_srmc;
                        polling_head = (polling_head + 1) % SRMC_POLLING_CNT;
                    }
                }
                else
                {
                    // poll完了，出队
                    in_queue[pre_srmc->srmc_idx % (CQ_NUM)] = 0;
                }
            }

            // 2. 无WQE时跳过当前等级
            if (!level_wqe_cnt)
            {
                DEBUG_LOG("level=%d no wqe (user=%d, kernel=%d)",
                          level, user_level_val, kernel_level_table[level_table_idx]);
                continue;
            }

            // 3. 遍历用户线程QP（优化1：预计算k的初始值）
            int k = level * num_sched + id + (srm_fastrand(&polling_seed) % num_user_threads) * num_thread_qps_per_sched;
            for (m = 0; m < num_user_threads; m++, k = (k + num_thread_qps) % sched_group.sqb_cnt)
            {
                // 预计算n的索引（避免重复计算）
                const int n = CALC_N(k, num_thread_qps_per_sched, level, num_user_threads, id_per_thread_qp_nums);
                const int user_table_val = smp_load_acquire(&user_wqe_table[n]);
                const int kernel_table_val = kernel_wqe_table[n];

                // 无WQE时跳过当前QP
                if (user_table_val == kernel_table_val)
                {

                    continue;
                }

                // 4. 读取SQB（优化7：检查空指针）
                struct mlx5_ib_sqbuf *sqb = sched_group.sqb_arr[k];
                if (unlikely(!sqb))
                {
                    pr_err_once("[id=%d] sqb[%d] is NULL", id, k);
                    continue;
                }

                // 5. 读取用户态WQE（优化8：提前计算偏移）
                void *seg, *useg;
                struct mlx5_wqe_xrc_seg *xrc, *uxrc;
                struct mlx5_wqe_raddr_seg *raddr, *uraddr;
                struct mlx5_wqe_data_seg *data, *udata;
                unsigned long flags;
                u32 mlx5_opcode;
                u32 opmod;
                void *cur_edge;
                u8 next_fence;
                u8 fence;
                u8 sig;
                struct mlx5_wqe_ctrl_seg *ctrl, *uctrl;
                int uidx_cqe = -1;

                const int uidx = sqb->cur_post & (sqb->wqe_cnt - 1); // 位运算替代取模
                useg = uctrl = (sqb->buf + (uidx << 6));             // 64B对齐，左移6位
                const u32 imm = smp_load_acquire(&uctrl->imm);

                if (unlikely(!imm))
                {
                    pr_err_once("[id=%d] sqb[%d] imm is 0 (uidx=%d)", id, k, uidx);
                    continue;
                }

                useg += sizeof(struct mlx5_wqe_ctrl_seg);
                uxrc = (struct mlx5_wqe_xrc_seg *)useg;
                useg += sizeof(struct mlx5_wqe_xrc_seg);
                uraddr = (struct mlx5_wqe_raddr_seg *)useg;
                useg += sizeof(struct mlx5_wqe_raddr_seg);
                udata = (struct mlx5_wqe_data_seg *)useg;

                const uint32_t length = ntohl(udata->byte_count);
                DEBUG_LOG("length:%d\n", length);

                // -------------------------- 6. 处理WQE（优化9：锁外预处理） --------------------------
                // 预处理GID和WQE长度（锁外操作，减少锁持有时间）
                ((char *)(&imm))[3] = 1;
                memcpy(gid.raw + 12, &imm, 4);

                // 查找目标SRMC（锁外操作）
                uint32_t hash_id = sched_hash_ip((char *)&imm, NUM_SRMC);
                rd = prandom_u32_max(num_kqps);

                for (i = 0; i < NUM_SRMC; i++)
                {
                    j = (i + hash_id) % NUM_SRMC;
                    srmc = (length > MESSAGE_SIZE_THRESHOLD ? sched->srmc_large_tb[j] : sched->srmc_small_tb[j]);
                    if (unlikely(srmc == NULL))
                    {
                        pr_err("Unexpected:No srmc found for this wr\n");
                        goto err;
                    }
                    if (memcmp(srmc->dgid.raw, gid.raw, sizeof(srmc->dgid.raw)) == 0)
                    {
                        DEBUG_LOG("found srmc,gid.interface_id:%llx,subnet_prefix:%llx\n", srmc->dgid.global.interface_id, srmc->dgid.global.subnet_prefix);
                        if (unlikely(!srmc->ini_cb.qp))
                        {
                            pr_err("Unexpected:ini qp for this srmc is NULL\n");
                            goto err;
                        }
                        found = 1;
                        j = (j + rd) % NUM_SRMC; // 随机选择一个srmc
                        srmc = (length > MESSAGE_SIZE_THRESHOLD ? sched->srmc_large_tb[j] : sched->srmc_small_tb[j]);
                        break;
                    }
                }
                if (unlikely(!found))
                {
                    pr_err("Unexpected:No srmc found for this wr\n");
                    goto err;
                }


                // 更新WQE统计（锁外操作）
                wqe_tot_sz -= cur_wqes[wqe_cur_idx];
                wqe_tot_sz += length;
                cur_wqes[wqe_cur_idx] = length;
                wqe_cur_idx = (wqe_cur_idx + 1) % WQES_ARR_SZ;
                srmc->cul_pending_bytes += length;
                srmc->pending_bytes += length;
                tfree = 0;

                // -------------------------- 7. 临界区操作（优化10：精简锁） --------------------------
                struct mlx5_ib_qp *qp = srmc->ini_cb.qp;
                if (unlikely(qp == NULL))
                {
                    pr_err_once("[id=%d] srmc[%d] qp is NULL", id, srmc->srmc_idx);
                    continue;
                }

                // 加锁：仅保护原子操作
                spin_lock_irqsave(&qp->sq.lock, flags);
                next_fence = 0;
                fence = qp->next_fence;

                if (unlikely(mlx5r_wq_overflow(&qp->sq, 1, qp->ibqp.send_cq)))
                {
                    pr_err("sq overflow\n");
                    spin_unlock_irqrestore(&qp->sq.lock, flags);
                    goto err;
                }

                int idx = qp->sq.cur_post & (qp->sq.wqe_cnt - 1);
                DEBUG_LOG("srmc qp's wqe_cnt:%d\n", srmc->ini_cb.qp->sq.wqe_cnt);
                ctrl = seg = mlx5_frag_buf_get_wqe(&qp->sq.fbc, idx);
                memcpy(ctrl, uctrl, sizeof(struct mlx5_wqe_ctrl_seg));
                ctrl->imm = 0;

                seg += sizeof(struct mlx5_wqe_ctrl_seg);
                // fence不管，可以吗?
                qp->sq.wr_data[idx] = 0;
                cur_edge = qp->sq.cur_edge;

                xrc = (struct mlx5_wqe_xrc_seg *)seg;
                memcpy(xrc, uxrc, sizeof(struct mlx5_wqe_xrc_seg));
                seg += sizeof(struct mlx5_wqe_xrc_seg);
                raddr = (struct mlx5_wqe_raddr_seg *)seg;
                memcpy(raddr, uraddr, sizeof(struct mlx5_wqe_raddr_seg));
                raddr->reserved = 0;
                seg += sizeof(struct mlx5_wqe_raddr_seg);
                data = (struct mlx5_wqe_data_seg *)seg;
                // handle_post_send_edge(&qp->sq, &seg, size,
                //     &cur_edge);//应该不需要？
                memcpy(data, udata, sizeof(struct mlx5_wqe_data_seg));
                seg += sizeof(struct mlx5_wqe_data_seg);

                qp->next_fence = next_fence;
                ctrl->opmod_idx_opcode = be32_to_cpu(ctrl->opmod_idx_opcode);
                mlx5_opcode = ctrl->opmod_idx_opcode & 0xFF;   // 低 8 位是 mlx5_opcode
                opmod = (ctrl->opmod_idx_opcode >> 24) & 0xFF; // 高 8 位是 opmod

                ctrl->opmod_idx_opcode = cpu_to_be32(((u32)(qp->sq.cur_post) << 8) |
                                                     mlx5_opcode | ((u32)opmod << 24));
                ctrl->qpn_ds = cpu_to_be32((be32_to_cpu(ctrl->qpn_ds) & 0xFF) |
                                           (qp->trans_qp.base.mqp.qpn << 8));
                ctrl->fm_ce_se |= fence;
                ctrl->fm_ce_se |= qp->sq_signal_bits;

                DEBUG_LOG("ctrl->fe_ce_se:%d,sq's signaled bits:%d\n", ctrl->fm_ce_se, qp->sq_signal_bits);
                DEBUG_LOG("qp's fence:%d\n", qp->next_fence);
                if ((ctrl->fm_ce_se & MLX5_WQE_CTRL_CQ_UPDATE))
                {
                    to_user = 1;
                }
                else
                {
                    to_user = 0;
                    if ((int)(qp->sq.head - qp->sq.tail + 1) >= qp->sq.max_post)
                    {
                        // 当前wqe发完之后该内核qp就满了
                        ctrl->fm_ce_se |= MLX5_WQE_CTRL_CQ_UPDATE;
                        // pr_info("insert kernel cqe\n");
                    }
                }
                sig = ctrl->fm_ce_se & MLX5_WQE_CTRL_CQ_UPDATE;

                if (sig)
                {
                    if (unlikely(free_cqe_cnt <= 0))
                    {
                        pr_err("free_cqe_cnt <= 0, cq exceed\n");
                        goto err;
                    }
                    uidx_cqe = free_cqe_idx[--free_cqe_cnt];
                    qp->sq.wrid[idx] = uidx_cqe;
                }

                qp->sq.w_list[idx].opcode = mlx5_opcode;
                qp->sq.wqe_head[idx] = qp->sq.head + 1;
                qp->sq.cur_post++;
                qp->sq.w_list[idx].next = qp->sq.cur_post;

                qp->sq.cur_edge = (unlikely(seg == cur_edge)) ? get_sq_edge(&qp->sq, qp->sq.cur_post &
                                                                                         (qp->sq.wqe_cnt - 1))
                                                              : cur_edge;

                DEBUG_LOG("rdma_wr.wr.wr_id:%llu\n", qp->sq.wrid[idx]);
                // TODO:cur_edge

                // ring doorbell
                mlx5r_ring_db(qp, 1, ctrl);
                // print_wqe_info(ctrl, sizeof(struct mlx5_wqe_ctrl_seg) +
                //  sizeof(struct mlx5_wqe_xrc_seg) +
                //  sizeof(struct mlx5_wqe_raddr_seg) +
                //   sizeof(struct mlx5_wqe_data_seg));
                spin_unlock_irqrestore(&qp->sq.lock, flags);

                // // -------------------------- 8. 锁外处理（优化10：减少锁持有时间） --------------------------
                // // 复制WQE数据（锁外操作）
                // memcpy(ctrl, uctrl, sizeof(*ctrl));
                // ctrl->imm = 0;
                // ctrl->fm_ce_se |= fence | qp->sq_signal_bits;
                // struct mlx5_wqe_xrc_seg *xrc = (void *)(ctrl + 1);
                // struct mlx5_wqe_raddr_seg *raddr = (void *)(xrc + 1);
                // struct mlx5_wqe_data_seg *data = (void *)(raddr + 1);
                // memcpy(xrc, uxrc, sizeof(*xrc));
                // memcpy(raddr, uraddr, sizeof(*raddr));
                // raddr->reserved = 0;
                // memcpy(data, udata, sizeof(*data));

                // // 更新WQE opcode和QPN（锁外操作）
                // mlx5_opcode = (be32_to_cpu(ctrl->opmod_idx_opcode)) & 0xFF;
                // opmod = (be32_to_cpu(ctrl->opmod_idx_opcode) >> 24) & 0xFF;
                // ctrl->opmod_idx_opcode = cpu_to_be32(((u32)(qp->sq.cur_post - 1) << 8) | mlx5_opcode | ((u32)opmod << 24));
                // ctrl->qpn_ds = cpu_to_be32((be32_to_cpu(ctrl->qpn_ds) & 0xFF) | (qp->trans_qp.base.mqp.qpn << 8));

                // // 触发Doorbell（锁外操作）
                // mlx5r_ring_db(qp, 1, ctrl);
                // DEBUG_LOG("send wqe (sqb=%d, qp=%d, length=%d, uidx_cqe=%d)",
                //           k, qp->trans_qp.base.mqp.qpn, length, uidx_cqe);

                // -------------------------- 9. 处理CQE入队（优化3：调用内联函数） --------------------------
                if (sig && uidx_cqe >= 0)
                {
                    // SRMC入队（避免重复入队）
                    const int cq_idx = CQ_MOD(srmc->srmc_idx);
                    if (!in_queue[cq_idx])
                    {
                        if (unlikely(pre_srmcs[polling_head] != NULL))
                        {
                            pr_err_once("[id=%d] pre_srmcs queue overflow", id);
                            pre_srmc = pre_srmcs[polling_tail];
                            pre_srmcs[polling_tail] = NULL;
                            polling_tail = (polling_tail + 1) % SRMC_POLLING_CNT;
                            // 此时队列满，必须poll到,此时head = (tail-1+polling_cnt)%polling_cnt
                            while ((ret = srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, &free_cqe_cnt)) != -1)
                            {
                                ;
                            }
                            in_queue[cq_idx] = 0;
                        }
                        if (cq_srmc_tb[cq_idx] == NULL)
                            cq_srmc_tb[cq_idx] = srmc;

                        pre_srmcs[polling_head] = cq_srmc_tb[cq_idx];
                        polling_head = (polling_head + 1) % SRMC_POLLING_CNT;
                        in_queue[cq_idx] = 1;
                    }
                    pre_srmc = cq_srmc_tb[cq_idx];

                    DEBUG_LOG("send signaled\n");
                    pre_srmc->wqe_infos[uidx_cqe].qpn = sqb->qpn;
                    pre_srmc->wqe_infos[uidx_cqe].wqe_counter = sqb->cur_post; // 当前cur_post提前++了，所以减1
                    pre_srmc->wqe_infos[uidx_cqe].pending_bytes = pre_srmc->cul_pending_bytes;
                    pre_srmc->wqe_infos[uidx_cqe].sqb = sqb;
                    pre_srmc->wqe_infos[uidx_cqe].to_user = to_user;
                    pre_srmc->wqe_infos[uidx_cqe].byte_cnt = length;
                    pre_srmc->wqe_infos[uidx_cqe].valid = 1;
                    pre_srmc->sig_cnt++;
                    pre_srmc->cul_pending_bytes = 0;
                }
                // -------------------------- 10. 收尾（优化11：提前退出循环） --------------------------
                smp_store_release(&kernel_wqe_table[n], kernel_table_val + 1);
                smp_store_release(&uctrl->imm, 0);
                sqb->cur_post++;
                kernel_level_table[level_table_idx]++;
                send_ok = 1;
                DEBUG_LOG("send finished\n");

                // 每发一个wqe再polling一次
                pre_srmc = pre_srmcs[polling_tail];
                pre_srmcs[polling_tail] = NULL;
                if (pre_srmc)
                {
                    polling_tail = (polling_tail + 1) % SRMC_POLLING_CNT;
                    // ret = srm_poll_srmc_once_debug(pre_srmc, wc, cqe, filp, &pos, buf, &start_cycles_cq, &end_cycles_cq,free_cqe_idx,&free_cqe_cnt); // 文件
                    ret = srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, &free_cqe_cnt);
                    if (pre_srmc->sig_cnt)
                    {
                        // 这次没poll完
                        if (unlikely(pre_srmcs[polling_head] != NULL))
                        {
                            pr_info("cq polling queue exceed queue length\n");
                            // 此时polling队列满，必须poll完
                            while (pre_srmc->sig_cnt)
                            {
                                srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, &free_cqe_cnt);
                            }
                            in_queue[pre_srmc->srmc_idx % (CQ_NUM)] = 0;
                        }
                        else if (unlikely(pre_srmc->sig_cnt >= SQ_DEPTH || (int)(srmc->ini_cb.qp->sq.head - srmc->ini_cb.qp->sq.tail) >= srmc->ini_cb.qp->sq.max_post))
                        {
                            if (pre_srmc->sig_cnt >= SQ_DEPTH)
                                pr_info("cq queue exceed SQ_DEPTH00\n");
                            else
                            {
                                pr_info("exceed max_post00,sq.head:%d, sq.tail:%d,max_post:%d",
                                        pre_srmc->ini_cb.qp->sq.head, pre_srmc->ini_cb.qp->sq.tail, pre_srmc->ini_cb.qp->sq.max_post);
                            }
                            // //cqe队列满或者sq队列满，必须poll到一个以上,让sig_cnt小于SQ_DEPTH
                            while (pre_srmc->sig_cnt >= SQ_DEPTH || srmc->ini_cb.qp->sq.head - srmc->ini_cb.qp->sq.tail >= srmc->ini_cb.qp->sq.max_post)
                            {
                                srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, &free_cqe_cnt);
                            }
                            if (!pre_srmc->sig_cnt)
                            {
                                // poll完
                                in_queue[pre_srmc->srmc_idx % (CQ_NUM)] = 0;
                            }
                            else
                            {
                                // 没poll完，重新加入队列
                                pre_srmcs[polling_head] = pre_srmc;
                                polling_head = (polling_head + 1) % SRMC_POLLING_CNT;
                            }
                        }
                        else
                        {
                            // 重新加入队列
                            pre_srmcs[polling_head] = pre_srmc;
                            polling_head = (polling_head + 1) % SRMC_POLLING_CNT;
                        }
                    }
                    else
                    {
                        // poll完了，出队
                        in_queue[pre_srmc->srmc_idx % (CQ_NUM)] = 0;
                    }
                }
                break; // 处理完成，退出内层循环
            }

            if (send_ok)
                break; // 处理完成，退出外层循环
        }

        // 11. 空转计数与睡眠（优化5：动态睡眠）
        cnt += tfree;
        if (cnt % 1000000 == 0)
        {
            cnt++;
            msleep(0); // 每百万次空转睡眠1ms，减少CPU占用
        }
    }

// -------------------------- 5. 资源释放与退出（内核规范：避免内存泄漏） --------------------------
out:
    DEBUG_LOG("exit normally");
err_mem:
    kfree(sched_id);
    kfree(cqe);
    kfree(wc);
    kfree(pre_srmcs);
    kfree(in_queue);
    kfree(free_cqe_idx);
    kfree(level_qp_st_arr);
    if (sched)
        sched->task = NULL;
    return 0;
err:
    pr_err("[id=%d] exit in error", id);
    goto out;
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

void mlx5_ib_sched_exit(struct mlx5_ib_sched_group *sched_group)
{
    struct mlx5_ib_sqbuf *sqb;
    struct mlx5_ib_cqbuf *cqb;
    struct mlx5_ib_srmc *srmc;
    struct mlx5_ib_sched *sched;
    int npages;
    int i, j;

    for (i = 0; i < sched_group->num_sched; i++)
    {
        DEBUG_LOG("Ready to stop sched->task %d\n", i);
        sched = &sched_group->scheds[i];
        if (sched->task)
        {
            kthread_stop(sched->task);
            sched->task = NULL;
        }
        mutex_lock(&sched->srmc_lock);
        for (j = 0; j < NUM_SRMC; j++)
        {
            srmc = sched->srmc_small_tb[j];
            if (srmc == NULL)
            {
                continue;
            }
            DEBUG_LOG("srmc ini_cb state:%d\n", srmc->ini_cb.state);
            // if (srmc->ini_cb.state == CONNECTED)//可能在event处理过程中发现state是CONNECTED，造成重复释放。如何解决？
            // {
            //     rdma_disconnect(srmc->ini_cb.cm_id);
            //     // ib_sched_free_buf(&srmc->ini_cb);
            //     ib_destroy_qp(&srmc->ini_cb.qp->ibqp);
            //     ib_destroy_cq(srmc->ini_cb.cq);
            // }
            if (srmc->ini_cb.cm_id)
            {
                rdma_disconnect(srmc->ini_cb.cm_id);
                ib_destroy_qp(&srmc->ini_cb.qp->ibqp);
                // ib_destroy_cq(srmc->ini_cb.cq);
                //  ib_dealloc_pd(srmc->ini_cb.pd);
                rdma_destroy_id(srmc->ini_cb.cm_id);
            }

            kfree(srmc);
        }

        for (j = 0; j < NUM_SRMC; j++)
        {
            srmc = sched->srmc_large_tb[j];
            if (srmc == NULL)
            {
                continue;
            }
            // if (srmc->ini_cb.state == CONNECTED)
            // {
            //     rdma_disconnect(srmc->ini_cb.cm_id);
            //     // ib_sched_free_buf(&srmc->ini_cb);
            //     ib_destroy_qp(&srmc->ini_cb.qp->ibqp);
            //     ib_destroy_cq(srmc->ini_cb.cq);
            // }
            if (srmc->ini_cb.cm_id)
            {
                rdma_disconnect(srmc->ini_cb.cm_id);
                ib_destroy_qp(&srmc->ini_cb.qp->ibqp);
                // ib_destroy_cq(srmc->ini_cb.cq);
                //  ib_dealloc_pd(srmc->ini_cb.pd);
                rdma_destroy_id(srmc->ini_cb.cm_id);
            }
            kfree(srmc);
        }
        mutex_unlock(&sched->srmc_lock);
        DEBUG_LOG("clean thread %d srmc success\n", i);
    }

    for (i = 0; i < sched_group->num_sched; i++)
    {
        for (j = 0; j < (CQ_NUM); j++)
            // free cq
            if (shared_cq[i][j])
                ib_destroy_cq(shared_cq[i][j]);
    }

    // clean up srm qp table
    if (user_wqe_table)
    {
        npages = ((num_table_qp * sizeof(user_wqe_table[0])) + PAGE_SIZE - 1) / PAGE_SIZE;
        vunmap(user_wqe_table);
        put_user_pages(user_wqe_pages, npages);
        kfree(user_wqe_pages);
    }

    if (user_level_table)
    {
        npages = ((num_table_level * sizeof(user_level_table[0])) + PAGE_SIZE - 1) / PAGE_SIZE;
        vunmap(user_level_table);
        put_user_pages(user_level_pages, npages);
        kfree(user_level_pages);
    }
    if (user_idx_table)
    {
        npages = ((num_table_level * sizeof(user_idx_table[0])) + PAGE_SIZE - 1) / PAGE_SIZE;
        vunmap(user_idx_table);
        put_user_pages(user_idx_pages, npages);
        kfree(user_idx_pages);
    }
    // cleanup srm qp
    mutex_lock(&sched_group->sq_lock);
    for (i = 0; i < sched_group->sqb_cnt; i++)
    {
        sqb = sched_group->sqb_arr[i];
        sched_group->sqb_arr[i] = NULL;
        if (sqb == NULL)
            continue;
        vunmap(sqb->buf);
        npages = (sqb->sq_size + PAGE_SIZE - 1) / PAGE_SIZE;
        for (i = 0; i < npages; i++)
            put_page(sqb->pages[i]);
        kfree(sqb->pages);
        kfree(sqb);
    }
    mutex_unlock(&sched_group->sq_lock);
    DEBUG_LOG("clean sqb success\n");

    mutex_lock(&sched_group->cq_lock);
    for (i = 0; i < sched_group->cqb_cnt; i++)
    {
        cqb = sched_group->cqb_arr[i];
        sched_group->cqb_arr[i] = NULL;
        if (cqb == NULL)
            continue;
        vunmap(cqb->buf);
        npages = (cqb->cq_size + PAGE_SIZE - 1) / PAGE_SIZE;
        for (i = 0; i < npages; i++)
            put_page(cqb->pages[i]);
        kfree(cqb->pages);
        kfree(cqb);
    }
    mutex_unlock(&sched_group->cq_lock);
    DEBUG_LOG("clean cqb success\n");

    kfree(sched_group->scheds);

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
                srmc = sched->srmc_small_tb[j];
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
            for (j = 0; j < NUM_SRMC; j++)
            {
                srmc = sched->srmc_large_tb[j];
                if (srmc == NULL)
                {
                    continue;
                }
                if (srmc->tgt_cb.state == CONNECTED)
                {
                    rdma_disconnect(srmc->tgt_cb.cm_id);
                    ib_destroy_qp(&srmc->tgt_cb.qp->ibqp);
                    ib_destroy_cq(srmc->tgt_cb.cq);
                    ib_dealloc_pd(srmc->tgt_cb.pd);
                }
                if (srmc->tgt_cb.cm_id)
                {
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
}
void mlx5_ib_gid2ip(char addr[4], union ib_gid *gid)
{
    memcpy(addr, gid->raw + 12, 4);
}
// return 0 means xrc exists, other means xrc not exists
int is_xrc_exists(struct mlx5_ib_sched *sched, struct ib_pd *pd, union ib_gid *dgid, int flags, int qpn)
{

    DEBUG_LOG("in is_xrc_exists,gid.in_id = %llx, gid.subnet = %llx\n", dgid->global.interface_id, dgid->global.subnet_prefix);
    DEBUG_LOG("gid.raw[15]:%u\n", dgid->raw[15]);
    struct mlx5_ib_srmc *srmc_small, *srmc_large;
    int ret = 1;
    int i, j;
    int hash_id;
    int has_srmc = 0;
    hash_id = sched_hash_ip((char *)dgid->raw + 12, NUM_SRMC);
    mutex_lock(&sched->srmc_lock);
    for (i = 0; i < NUM_SRMC; i++)
    {
        j = (hash_id + i) % NUM_SRMC;
        srmc_small = sched->srmc_small_tb[j], srmc_large = sched->srmc_large_tb[j];
        if (srmc_small == NULL)
        {
            break;
        }
        if (memcmp(srmc_small->dgid.raw, dgid->raw, sizeof(srmc_small->dgid.raw)) == 0)
        {
            if (flags == SRMC_CREATE_FLAG_INIT_QP)
            {
                if (srmc_small->ini_cb.refcnt == 0)
                {
                    srmc_small->ini_cb.refcnt = 1;
                    srmc_large->ini_cb.refcnt = 1;
                    ret = 1;
                }
                else
                {
                    srmc_small->ini_cb.refcnt++;
                    srmc_large->ini_cb.refcnt++;
                    ret = 0;
                }
            }
            has_srmc = 1;
            break;
        }
    }

    if (!has_srmc)
    {
        if (sched->srmc_small_tb[j] != NULL || sched->srmc_large_tb[j] != NULL)
        {
            pr_err("srmc queue is full\n");
            mutex_unlock(&sched->srmc_lock);
            return -1;
        }
        for (i = 0; i < num_kqps; i++)
        {
            // srmc no exists
            srmc_small = kzalloc(sizeof(struct mlx5_ib_srmc), GFP_KERNEL);
            srmc_large = kzalloc(sizeof(struct mlx5_ib_srmc), GFP_KERNEL);
            memcpy(srmc_small->dgid.raw, dgid->raw, sizeof(srmc_small->dgid.raw));
            memcpy(srmc_large->dgid.raw, dgid->raw, sizeof(srmc_large->dgid.raw));
            if (flags == SRMC_CREATE_FLAG_INIT_QP)
            {
                srmc_small->ini_cb.refcnt = 1;
                srmc_large->ini_cb.refcnt = 1;
            }
            srmc_small->idx = 2 * j - 1;
            srmc_large->idx = 2 * j;

            srmc_small->srmc_idx = i;
            srmc_large->srmc_idx = i;

            sched->srmc_small_tb[j] = srmc_small;
            sched->srmc_large_tb[j] = srmc_large;

            sched->srmc_cnt[0]++;
            sched->srmc_cnt[1]++;

            mutex_unlock(&sched->srmc_lock);
            if (1)
            {
                ret = create_srmc_qp_cm(srmc_small, pd, dgid, MESSAGE_SIZE_SMALL, sched->id);
                pr_info("create_srmc_qp_cm small ret:%d\n", ret);
                if (ret)
                {
                    ret = create_srmc_qp_cm(srmc_large, pd, dgid, MESSAGE_SIZE_LARGE, sched->id);
                    pr_info("create_srmc_qp_cm large ret:%d\n", ret);
                }
            }
            else
            {

                while (!(srmc_large->ini_cb.state == CONNECTED))
                {
                    msleep(0);
                }
            }
            j = (j + 1) % NUM_SRMC;
            mutex_lock(&sched->srmc_lock);
        }
    }

    mutex_unlock(&sched->srmc_lock);

    DEBUG_LOG("out is_xrc_exists,ret:%d\n", ret);
    return ret;
}

// int mlx5_ib_sched_ins_ah_id(struct mlx5_ib_sched* sched,union ib_gid* dgid,int ah_id){
//     struct mlx5_ib_srmc *srmc;
//     mutex_lock(&sched->srmc_lock);
//     for(srmc = sched->srmc_head;srmc;srmc=srmc->next){
//         if(memcmp(&srmc->dgid,dgid,sizeof (union ib_gid))==0){
//             srmc->ah_id = ah_id;
//             mutex_unlock(&sched->srmc_lock);
//             return 0;
//         }
//     }
//     mutex_unlock(&sched->srmc_lock);
//     return -1;

// }

// int mlx5_ib_destroy_srmc(struct mlx5_ib_sched* sched,int ah_id){
//     struct mlx5_ib_srmc *srmc,*tmp;
//     mutex_lock(&sched->srmc_lock);
//     if(sched->srmc_head==NULL){
//         goto err;
//     }
//     if(sched->srmc_head->ah_id == ah_id){
//         srmc = sched->srmc_head;
//         sched->srmc_head = srmc->next;
//     }else{
//         for(srmc=sched->srmc_head;srmc->next;srmc=srmc->next){
//             if(srmc->next->ah_id == ah_id){
//                 break;
//             }
//         }
//         if(srmc->next==NULL)
//             goto err;
//         tmp = srmc->next;
//         srmc->next = tmp->next;
//         srmc = tmp;
//     }
//     //destroy qp
//     if(srmc->init_qp)
//         ib_destroy_qp_user(&srmc->init_qp->ibqp,NULL);
//     if(srmc->tgt_qp)
//         ib_destroy_qp_user(&srmc->tgt_qp->ibqp,NULL);

//     kfree(srmc);
//     //success
//     mutex_unlock(&sched->srmc_lock);
//     return 0;
// err:
//     mutex_unlock(&sched->srmc_lock);
//     return -1;
// }

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

// //return 0 if success, return -1 if failed. return >0 for qpn.
// int mlx5_ib_create_srmc_qp(struct mlx5_ib_sched* sched,struct mlx5_ib_srmc *srmc,struct ib_pd *pd,int flags,int qpn){
//     struct ib_qp *qp;
//     struct ib_qp_init_attr create_attr;
//     struct ib_cq *cq;
//     struct ib_cq_init_attr cq_attr;
//     struct ib_xrcd *xrcd;
//     struct ibv_qp_info local_qp_info,remote_qp_info;
//     memset(&local_qp_info,0,sizeof(local_qp_info));
//     memset(&remote_qp_info,0,sizeof(remote_qp_info));
//     memset(&cq_attr,0,sizeof cq_attr);
//     const int sq_depth = 256;
//     const int cq_depth = 256;
//     const int rq_depth = 256;
//     const int psn = 3100;
//     const int max_rd_atomic = 16;
//     const int port_num = 1;
//     memset(&create_attr,0,sizeof create_attr);
//     struct ib_qp_attr conn_attr;
//     int ret;
//     // struct ib_device *ibdev;
//     print_pd_info(pd);

//     cq_attr.cqe = cq_depth;
//     cq_attr.comp_vector = 0;// CPU 0
//     cq = ib_create_cq(pd->device,NULL,NULL,NULL,&cq_attr);
//     if(!cq){
//         pr_err("Create CQ false\n");
//         return -1;
//     }
//     if(flags == SRMC_CREATE_FLAG_INIT_QP){
//         if(srmc->ini_cb.qp){
//             pr_err("Unexpected:init qp exists\n");
//             return -1;
//         }
//         // //alloc mr
//         // srmc->ini_mr = ib_alloc_mr(pd,IB_MR_TYPE_MEM_REG,1);
//         // if(srmc->ini_mr==NULL){
//         //     pr_err("Alloc mr false\n");
//         //     return -1;
//         // }

//         //alloc mr
//         if(mlx5_sched_alloc_mr(&srmc->ini_cb,pd)){
//             pr_err("mr alloc false");
//             return -1;
//         }

//         create_attr.qp_type = IB_QPT_XRC_INI;
//         create_attr.send_cq = cq;

//         // create_attr.qp_type = IB_QPT_RC;
//         // create_attr.recv_cq = cq;//RC
//         // create_attr.cap.max_recv_wr = 1;
//         // create_attr.cap.max_recv_sge = 1;//RC

//         create_attr.cap.max_send_wr = sq_depth;
//         create_attr.cap.max_send_sge = 1;
//         create_attr.cap.max_inline_data = 128;
//         // create_attr.cap.max_rdma_ctxs = 1;
//         create_attr.sq_sig_type = IB_SIGNAL_REQ_WR;
//         pr_info("ready to ib_create_qp_kernel\n");
//         qp = ib_create_qp_kernel(pd,&create_attr,"scheduler");
//         pr_info("flags == SRMC_CREATE_FLAG_INIT_QP create qp_kernel\n");
//         if(!qp){
//             pr_err("Create Kernel qp false\n");
//             return -1;
//         }
//         local_qp_info.qpn = qp->qp_num;
//         // 打印 subnet_prefix 和 interface_id（需注意大端序转换）
//         printk(KERN_INFO "Subnet Prefix: %llx\n", srmc->dgid.global.subnet_prefix);
//         printk(KERN_INFO "Interface ID: %llx\n", srmc->dgid.global.interface_id);
//         // memcpy(local_qp_info.gid.raw,srmc->dgid.raw,sizeof(union ib_gid));//now just the same server,TODO:change to local gid.
//         printk(KERN_INFO "local_qp_info GID (raw): %u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u\n",
//             local_qp_info.gid.raw[0], local_qp_info.gid.raw[1], local_qp_info.gid.raw[2], local_qp_info.gid.raw[3],
//             local_qp_info.gid.raw[4], local_qp_info.gid.raw[5], local_qp_info.gid.raw[6], local_qp_info.gid.raw[7],
//             local_qp_info.gid.raw[8], local_qp_info.gid.raw[9], local_qp_info.gid.raw[10], local_qp_info.gid.raw[11],
//             local_qp_info.gid.raw[12], local_qp_info.gid.raw[13], local_qp_info.gid.raw[14], local_qp_info.gid.raw[15]);

//         // ibdev = ib_device_get_by_name("mlx5_0", RDMA_DRIVER_MLX5);//内核打开设备，类似open_device
//         // 打印 ibdev->name 以确认设备名称
//         pr_info("Successfully got IB device: %s\n", pd->device->name);
//         // ret= ibdev->ops.query_gid(ibdev, port_num, 0, &local_qp_info.gid);//不适用于Rocev2
//         // ret = pd->device->ops.query_gid(pd->device,port_num,0,&local_qp_info.gid);
//         ret=rdma_query_gid(pd->device,port_num,0,&local_qp_info.gid);
//         if(ret){
//             pr_err("local query_gid false,error code:%d\n",ret);
//             return -1;
//         }
//         pr_info("local gid interface_id: 0x%llx, subnet_prefix:0x%llx\n",local_qp_info.gid.global.interface_id,local_qp_info.gid.global.subnet_prefix);
//         memset(&remote_qp_info,0,sizeof(remote_qp_info));

//         mutex_unlock(&sched->srmc_lock);//give the lock to server
//         int rtry_cnt = 0;
//         while(conn_server_kernel("127.0.0.1",12345,&local_qp_info,&remote_qp_info) < 0){
//             pr_err("conn_server_kernel failed\n");
//             rtry_cnt++;
//             if(rtry_cnt > 10){
//                 pr_err("conn_server_kernel failed %d times\n",rtry_cnt);
//                 mutex_lock(&sched->srmc_lock);
//                 return -1;
//             }
//         }
//         mutex_lock(&sched->srmc_lock);

//         srmc->ini_cb.qp = to_mqp(qp);
//         srmc->ini_cb.cq = to_mcq(cq);
//         pr_info("srmc->init_qp is OK\n");
//     }else{
//         if(srmc->tgt_cb.qp){
//             pr_err("Unexpected:tgt qp exists\n");
//             return -1;
//         }
//         // //alloc mr
//         // srmc->tgt_mr = ib_alloc_mr(pd,IB_MR_TYPE_MEM_REG,1);
//         // if(srmc->tgt_mr==NULL){
//         //     pr_err("Alloc mr false\n");
//         //     return -1;
//         // }

//         if(mlx5_sched_alloc_mr(&srmc->tgt_cb,pd)){
//             pr_err("mr alloc false");
//             return -1;
//         }

//         //为了统一modify操作赋值remote qp info
//         remote_qp_info.qpn = qpn;
//         remote_qp_info.gid.global.interface_id = srmc->dgid.global.interface_id;
//         remote_qp_info.gid.global.subnet_prefix = srmc->dgid.global.subnet_prefix;

//         create_attr.qp_type = IB_QPT_XRC_TGT;
//         create_attr.xrcd = xrcd;

//         create_attr.send_cq = cq;
//         create_attr.qp_type = IB_QPT_RC;
//         create_attr.cap.max_send_sge = 1;
//         create_attr.cap.max_send_wr = sq_depth;//RC

//         create_attr.recv_cq = cq;
//         create_attr.cap.max_recv_wr = rq_depth;
//         create_attr.cap.max_recv_sge = 1;
//         create_attr.cap.max_inline_data = 128;
//         pr_info("ready to ib_create_qp_kernel else\n");
//         qp = ib_create_qp_kernel(pd,&create_attr,"scheduler");
//         if(!qp){
//             pr_err("Create Kernel qp false\n");
//             return -1;
//         }
//         pr_info("flags != SRMC_CREATE_FLAG_INIT_QP create qp_kernel\n");
//         srmc->tgt_cb.qp = to_mqp(qp);
//     }
//     //modify qp
//     memset(&conn_attr,0,sizeof conn_attr);
//     conn_attr.qp_state = IB_QPS_INIT;
//     conn_attr.pkey_index = 0;
//     conn_attr.port_num = port_num;//is that ok?
//     conn_attr.qp_access_flags = IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ | IB_ACCESS_REMOTE_ATOMIC;
//     if (ib_modify_qp(qp, &conn_attr,
//             IB_QP_STATE | IB_QP_PKEY_INDEX | IB_QP_PORT |
//                 IB_QP_ACCESS_FLAGS)) {
//         pr_err("Failed to modify conn QP to INIT\n");
//         return -1;
//     }

//     memset(&conn_attr, 0, sizeof(struct ib_qp_attr));
//     conn_attr.qp_state = IB_QPS_RTR;
//     conn_attr.path_mtu = IB_MTU_1024;
//     conn_attr.dest_qp_num = remote_qp_info.qpn;
//     pr_info("remote qpn: %d\n",remote_qp_info.qpn);
//     conn_attr.rq_psn = psn;

//     //conn_attr.ah_attr.is_global = 1; RoCE or IB???
//     conn_attr.ah_attr.ib.dlid = 0;
//     conn_attr.ah_attr.sl = 0;
//     conn_attr.ah_attr.ib.src_path_bits = 0;
//     conn_attr.ah_attr.port_num = 1;  // Local port?is that ok?

//     struct ib_global_route *grh = &conn_attr.ah_attr.grh;
//     grh->dgid.global.interface_id = remote_qp_info.gid.global.interface_id;
//     grh->dgid.global.subnet_prefix = remote_qp_info.gid.global.subnet_prefix;
//     pr_info("remote gid interface_id: %llx, subnet_prefix:%llx\n",remote_qp_info.gid.global.interface_id,remote_qp_info.gid.global.subnet_prefix);
//     grh->sgid_index = 0;
//     grh->hop_limit = 1;

//     conn_attr.ah_attr.type = RDMA_AH_ATTR_TYPE_ROCE;
//     conn_attr.ah_attr.ah_flags = IB_AH_GRH;

//     int rtr_flags = IB_QP_STATE | IB_QP_AV | IB_QP_PATH_MTU | IB_QP_DEST_QPN |
//                     IB_QP_RQ_PSN;

//     //if(flags == SRMC_CREATE_FLAG_TGT_QP){
//         conn_attr.max_dest_rd_atomic = max_rd_atomic;
//         conn_attr.min_rnr_timer = 12;
//         rtr_flags |= IB_QP_MAX_DEST_RD_ATOMIC | IB_QP_MIN_RNR_TIMER;
//     //}

//     if (ib_modify_qp(qp, &conn_attr, rtr_flags)) {
//         pr_err("Failed to modify QP to RTR\n");
//         return -1;
//     }

//     memset(&conn_attr, 0, sizeof(conn_attr));
//     conn_attr.qp_state = IB_QPS_RTS;
//     conn_attr.sq_psn = psn;

//     int rts_flags = IB_QP_STATE | IB_QP_SQ_PSN;

//     conn_attr.timeout = 18;
//     conn_attr.retry_cnt = 30;//增大重试时间
//     conn_attr.rnr_retry = 30;
//     conn_attr.max_rd_atomic = max_rd_atomic;
//     conn_attr.max_dest_rd_atomic = max_rd_atomic;
//     rts_flags |= IB_QP_TIMEOUT | IB_QP_RETRY_CNT | IB_QP_RNR_RETRY |
//                 IB_QP_MAX_QP_RD_ATOMIC;

//     if (ib_modify_qp(qp, &conn_attr, rts_flags)) {
//         pr_err("Failed to modify QP to RTS\n");
//         return -1;
//     }

//     //reg mr
//     if(flags == SRMC_CREATE_FLAG_INIT_QP){
//         if(mlx5_sched_reg_mr(srmc->ini_cb.mr,srmc->ini_cb.qp,srmc->ini_cb.dma_buf,srmc->ini_cb.buf_sz,srmc->ini_cb.page_list_len)){
//             pr_err("reg mr false\n");
//             return -1;
//         }
//     }else{

//         if(mlx5_sched_reg_mr(srmc->tgt_cb.mr,srmc->tgt_cb.qp,srmc->tgt_cb.dma_buf,srmc->tgt_cb.buf_sz,srmc->tgt_cb.page_list_len)){
//             pr_err("reg mr false\n");
//             return -1;
//         }
//     }

//     pr_info("Successfully created SRMC\n");
//     return qp->qp_num;
// }
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
        srmc = (flags == MESSAGE_SIZE_LARGE ? sched->srmc_large_tb[j] : sched->srmc_small_tb[j]);
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
        srmc = (flags == MESSAGE_SIZE_LARGE ? sched->srmc_large_tb[j] : sched->srmc_small_tb[j]);
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
        sched->srmc_cnt[idx]++;
        memcpy(srmc->dgid.raw, dgid.raw, sizeof(srmc->dgid.raw));
        // 将srmc 加入到srmc_head中
        if (flags == MESSAGE_SIZE_LARGE)
        {
            sched->srmc_large_tb[j] = srmc;
        }
        else
        {
            sched->srmc_small_tb[j] = srmc;
        }
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

int create_srmc_qp_cm(struct mlx5_ib_srmc *srmc, struct ib_pd *pd, union ib_gid *dgid, int flags, int id)
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
    cb->txdepth = SQ_DEPTH;
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

    // reg mr and bind buf
    //  ret = mlx5_sched_reg_mr(cb->mr,cb->qp,cb->dma_buf,cb->buf_sz,cb->page_list_len);
    //  if(ret){
    //      pr_err("reg mr failed,error:%d\n",ret);
    //      goto err4;
    //  }
    //  DEBUG_LOG("client reg mr success\n");

    // struct ib_rdma_wr wr;
    // struct ib_send_wr bad_wr;
    // struct ib_sge sgl;
    // struct ib_wc wc;

    // memset(&wr,0,sizeof(wr));

    // sgl.addr = cb->dma_buf;
    // sgl.length = cb->buf_sz;
    // sgl.lkey = cb->mr->lkey;
    // wr.wr.opcode = IB_WR_RDMA_WRITE;
    // wr.wr.qp_type.xrc.remote_srqn = 1145;
    // wr.wr.send_flags = IB_SEND_SIGNALED;
    // wr.wr.sg_list = &sgl;
    // wr.wr.num_sge = 1;
    // ret = ib_post_send(cb->qp,&wr,&bad_wr);
    // if(ret){
    //     pr_err("ib_post_send failed,error:%d\n",ret);
    //     goto err4;
    // }

    // struct ib_sge sgl;
    // struct ib_recv_wr recv_wr,*bad_wr;
    // struct ib_wc wc;
    // memset(&recv_wr,0,sizeof recv_wr);
    // sgl.addr = cb->dma_buf;
    // sgl.length = sizeof(struct buf_info);
    // sgl.lkey = cb->mr->lkey;
    // recv_wr.num_sge = 1;
    // recv_wr.sg_list = &sgl;
    // if(ib_post_recv(cb->qp,&recv_wr,&bad_wr)){
    //     pr_err("post recv failed\n");
    //     goto err2;
    // }
    // int cqe_num = 0;
    // while(!cqe_num){
    //     cqe_num = ib_poll_cq(cb->cq,10,&wc);
    // }
    // DEBUG_LOG("poll cqe_num:%d,wc status:%d,opcode:%d\n",cqe_num,wc.status,wc.opcode);

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

int mlx5_ib_register_external_table(void *table, size_t size, struct page **pages,
                                    void *level_table, size_t level_size, struct page **level_pages,
                                    void *idx_table, size_t idx_size, struct page **idx_pages)
{
    user_wqe_table = (uint32_t *)table;
    user_wqe_pages = pages;
    if (size % sizeof(uint32_t) != 0)
    {
        pr_err("error:size can't be divided,size:%d\n", size);
    }
    num_table_qp = size / sizeof(uint32_t);
    pr_info("qp数量：%d\n", num_table_qp);

    user_level_table = (uint32_t *)level_table;
    user_level_pages = level_pages;
    if (level_size % sizeof(uint32_t) != 0)
    {
        pr_err("error:level_size can't be divided,level_size:%d\n", level_size);
    }
    num_table_level = level_size / sizeof(uint32_t);
    pr_info("level数量：%d\n", num_table_level);

    user_idx_table = (uint32_t *)idx_table;
    user_idx_pages = idx_pages;
    if (idx_size % sizeof(uint32_t) != 0)
    {
        pr_err("error:idx_size can't be divided,idx_size:%d\n", idx_size);
    }
    if (num_table_level != idx_size / sizeof(uint32_t))
    {
        pr_err("error:level_size != idx_size\n");
    }

    return 0;
}
EXPORT_SYMBOL_GPL(mlx5_ib_register_external_table);