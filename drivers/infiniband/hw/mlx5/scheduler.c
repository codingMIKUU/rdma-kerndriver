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
#include<rdma/ib_verbs.h>
#include "scheduler.h"
#include "qp.h"
#include "wr.h"
#include <linux/mlx5/device.h>
#include "user_verbs.h"
#include "conn.h"


int mlx5_ib_map_ubuf(struct mlx5_ib_sched* sched,unsigned long virt_addr,size_t size,int qpn,int cqn){
    pr_info("in mlx5_ib_map_ubuf\n");
    pr_info("内核态virt_addr:%px\n",virt_addr);
    struct mm_struct *mm = current->mm;
    int ret;
    int i;
    struct mlx5_ib_sqbuf *uq;
    size_t npages = (size +PAGE_SIZE-1)/PAGE_SIZE;
    struct page** pages = kmalloc_array(npages,sizeof(struct page *),GFP_KERNEL);
    if(!pages)
        return -ENOMEM;
    //**pages参数存储的是物理页（Physical Page）的元数据描述符，而非虚拟地址或物理地址的直接数值
    //可使用kmap(page)或vmap转换为内核态虚拟地址，page_to_phys(page)转换为物理地址
    ret = get_user_pages_fast(virt_addr,npages,FOLL_WRITE,pages);//pin user pages in memory, Returns number of pages pinned.
    if (ret < npages) {
        // 如果获取的页面数少于预期，释放资源并返回错误
        for (i = 0; i < ret; i++)
            put_page(pages[i]);
        kfree(pages);
        return -EFAULT;
    }
    mutex_lock(&sched->sq_lock);
    if(sched->sq_head == NULL){
        uq = sched->sq_head = kzalloc(sizeof(struct mlx5_ib_sqbuf),GFP_KERNEL);
    }else{
        uq = kzalloc(sizeof(struct mlx5_ib_sqbuf),GFP_KERNEL);
        uq->next = sched->sq_head->next;
        sched->sq_head->next = uq;
    }
    uq->cqn = cqn;
    uq->qpn = qpn;
    pr_info("内核态qpn:%d\n",uq->qpn);
    uq->sq_size = size;
    uq->buf = vmap(pages,npages,VM_MAP,PAGE_KERNEL);//map the pages(phys addr) to kernel space（virtual addr），非连续物理页映射到虚拟页
    uq->pages = pages;
    if(!uq->buf){
        for (i = 0; i < ret; i++)
            put_page(pages[i]);
        kfree(pages);
        kfree(uq);
        pr_info("fail to map sq buffer\n");
        mutex_unlock(&sched->sq_lock);
        return -ENOMEM;
    }
    pr_info("map sq buf success\n");
    mutex_unlock(&sched->sq_lock);

    return 0;
}

int mlx5_ib_map_cq_ubuf(struct mlx5_ib_sched* sched,unsigned long virt_addr,size_t size,int cqn){
    pr_info("in mlx5_ib_map_cq_ubuf\n");
    struct mm_struct *mm = current->mm;
    int ret;
    int i;
    struct mlx5_ib_cqbuf *uq;
    size_t npages = (size +PAGE_SIZE-1)/PAGE_SIZE;
    struct page** pages = kmalloc_array(npages,sizeof(struct page *),GFP_KERNEL);
    if(!pages)
        return -ENOMEM;
    ret = get_user_pages_fast(virt_addr,npages,FOLL_WRITE,pages);
    if (ret < npages) {
        // 如果获取的页面数少于预期，释放资源并返回错误
        for (i = 0; i < ret; i++)
            put_page(pages[i]);
        kfree(pages);
        return -EFAULT;
    }
    mutex_lock(&sched->cq_lock);
    if(sched->cq_head == NULL){
        uq = sched->cq_head = kzalloc(sizeof(struct mlx5_ib_cqbuf),GFP_KERNEL);
    }else{
        uq = kzalloc(sizeof(struct mlx5_ib_cqbuf),GFP_KERNEL);
        uq->next = sched->cq_head->next;
        sched->cq_head->next = uq;
    }
    uq->cqn = cqn;
    uq->cq_size = size;
    uq->buf = vmap(pages,npages,VM_MAP,PAGE_KERNEL);
    uq->pages = pages;
    if(!uq->buf){
        kfree(uq);
        for (i = 0; i < ret; i++)
            put_page(pages[i]);
        kfree(pages);
        pr_err("failed to map cf buffer\n");
        mutex_unlock(&sched->cq_lock);
        return -ENOMEM;
    }
    mutex_unlock(&sched->cq_lock);

    return 0;
}

int mlx5_ib_unmap_ubuf(struct mlx5_ib_sched* sched,int qpn){
    int cqn;
    struct mlx5_ib_sqbuf *sqb,*tmp;
    struct mlx5_ib_cqbuf *cqb,*tmp2;
    int npages;
    int i;
    mutex_lock(&sched->sq_lock);
    pr_info("mlx5_ib_unmap_ubuf清除映射资源\n");
    if(sched->sq_head==NULL){
        mutex_unlock(&sched->sq_lock);
        return 0;
    }
    if(sched->sq_head->qpn == qpn){
        cqn = sched->sq_head->cqn;
        sqb = sched->sq_head;
        sched->sq_head = sqb->next;
        vunmap(sqb->buf);
        npages = (sqb->sq_size +PAGE_SIZE-1)/PAGE_SIZE;
        for(i=0;i<npages;i++)
            put_page(sqb->pages[i]);
        kfree(sqb->pages);
        kfree(sqb);
    }
    else{
        for(sqb=sched->sq_head->next;sqb->next;sqb=sqb->next){
            if(sqb->next->qpn == qpn){
                cqn = sqb->next->cqn;
                tmp = sqb->next;
                sqb->next = sqb->next->next;
                vunmap(tmp->buf);
                npages = (tmp->sq_size +PAGE_SIZE-1)/PAGE_SIZE;
                for(i=0;i<npages;i++)
                    put_page(tmp->pages[i]);
                kfree(tmp->pages);
                kfree(tmp);
                break;
            }
        }
    }
    mutex_unlock(&sched->sq_lock);

    //Free cq
    mutex_lock(&sched->cq_lock);
    if(sched->cq_head==NULL){
        mutex_unlock(&sched->cq_lock);
        return 0;
    }
    if(sched->cq_head->cqn == cqn){
        cqb = sched->cq_head;
        sched->cq_head = cqb->next;
        vunmap(cqb->buf);
        npages = (cqb->cq_size +PAGE_SIZE-1)/PAGE_SIZE;
        for(i=0;i<npages;i++)
            put_page(cqb->pages[i]);
        kfree(cqb->pages);
        kfree(cqb);
    }
    else{
        for(cqb=sched->cq_head->next;cqb->next;cqb=cqb->next){
            if(cqb->next->cqn == cqn){
                tmp2 = cqb->next;
                cqb->next = cqb->next->next;
                vunmap(tmp2->buf);
                npages = (tmp2->cq_size +PAGE_SIZE-1)/PAGE_SIZE;
                for(i=0;i<npages;i++)
                    put_page(tmp2->pages[i]);
                kfree(tmp2->pages);
                kfree(tmp2);
                break;
            }
        }
    }
    mutex_unlock(&sched->cq_lock);
    return 0;

}

int scheduler_polling(void* data)
{
    int ret;
    struct mlx5_ib_sched* sched = (struct mlx5_ib_sched*) data;
    struct mlx5_ib_sqbuf* sqb;
    struct mlx5_ib_cqbuf* cqb;
    struct ibv_send_wr *ibv_wr;
    struct ib_rdma_wr rdma_wr;
    struct mlx5_ib_srmc *srmc;
    struct ib_send_wr *bad_wr;
    struct ib_wc wc;
    struct ibv_wc *uwc;
    void * cqe,*ucqe;
    int qpn;
    int cqn = -1;
    int nreq;
    struct mlx5_cqe64 *cqe64,*ucqe64;
    struct ib_sge sgl;
    memset(&rdma_wr,0,sizeof rdma_wr);
    
    while(!kthread_should_stop()){
        // pr_info("polling hhhhh\n");
        mutex_lock(&sched->sq_lock);
        for(sqb = sched->sq_head;sqb;sqb=sqb->next){//是否应该加一个检查该QP有几个WR，都拉下来的逻辑？
            ibv_wr = (struct ibv_send_wr*)(sqb->buf + sqb->cur_post * sizeof(struct ibv_send_wr));
            if(!ibv_wr->wr_id){
                continue;
            }
            pr_info("posting wr,sizeof wr is %u\n",sizeof(struct ibv_send_wr));
            pr_info("qpn:%d,cur_post:%d,wr_id:%llu\n",sqb->qpn,sqb->cur_post,ibv_wr->wr_id);
            //can just have one wr for now 
            rdma_wr.wr.wr_id = (((uint64_t)sqb->qpn) << 32) | sqb->cur_post;
            pr_info("rdma_wr.wr.wr_id:%llu\n",rdma_wr.wr.wr_id);
            memcpy(&sgl,&ibv_wr->sge,sizeof(struct ib_sge)); 

            pr_info("opcode:%u,send_flags:%u,imm_data:%u,srqn:%u,",ibv_wr->opcode,ibv_wr->send_flags,ibv_wr->imm_data,ibv_wr->qp_type.srm.remote_srqn);
            rdma_wr.wr.sg_list = &sgl;
            rdma_wr.wr.num_sge = 1;
            rdma_wr.wr.opcode = ibv_wr->opcode;
            rdma_wr.wr.send_flags =ibv_wr->send_flags;
            rdma_wr.wr.ex.imm_data = ibv_wr->imm_data;
            rdma_wr.wr.qp_type.xrc.remote_srqn = ibv_wr->qp_type.srm.remote_srqn;
            rdma_wr.remote_addr = ibv_wr->wr.rdma.remote_addr;
            rdma_wr.rkey = ibv_wr->wr.rdma.rkey;
            ibv_wr->wr_id = 0;
            sqb->cur_post++;
            pr_info("发送端的数据缓存区地址: %px\n", sgl.addr);
            pr_info("发送端要写入的缓存区地址: %px\n", rdma_wr.remote_addr);
            pr_info("服务端lkey：%d，收到的rkey：%d\n", sgl.lkey, rdma_wr.rkey);
            
            //find the srmc qp to send this req
            mutex_lock(&sched->srmc_lock);
            for(srmc=sched->srmc_head;srmc;srmc=srmc->next){
                if(memcmp(srmc->dgid.raw,ibv_wr->qp_type.srm.remote_gid.raw,sizeof(srmc->dgid.raw))==0){
                    pr_info("max_gs:%d\n,max_post:%d,fbc:%u\n",srmc->init_qp->sq.max_gs,srmc->init_qp->sq.max_post,srmc->init_qp->sq.fbc);
                    if(ret = ib_post_send(&srmc->init_qp->ibqp,&rdma_wr.wr,&bad_wr)){
                        pr_err("post send error:%d\n",ret);
                    }
                    pr_info("send finished\n");
                    srmc->sig_cnt+=(rdma_wr.wr.send_flags & IB_SEND_SIGNALED) ? 1 : 0;
                    break;
                }
            }
            mutex_unlock(&sched->srmc_lock);
            if(sqb->cur_post*sizeof(struct ibv_send_wr) >= sqb->sq_size){
                sqb->cur_post = 0;
            }
            
        }
        mutex_unlock(&sched->sq_lock);  
        // //TODO:poll xrc cq and distribute them
        mutex_lock(&sched->srmc_lock);
        for(srmc=sched->srmc_head;srmc;srmc=srmc->next){
            if(srmc->sig_cnt){
                memset(&wc,1,sizeof wc);
                int cqe_num=0;
                while(!cqe_num){
                    cqe_num=ib_poll_cq(srmc->init_qp->ibqp.send_cq,1,&wc);
                    // cqe_num = mlx5_ib_poll_cq_with_cqe(srmc->init_qp->ibqp.send_cq,1,&wc,&cqe);
                }
                pr_warn("cqe_num:%d,wc status:%d\n",cqe_num,wc.status);
                // ret = mlx5_ib_poll_cq_with_cqe(srmc->init_qp->ibqp.send_cq,1,&wc,&cqe);//is that ok to get cqe?
                // pr_warn("ret: %d, wc status:%d",ret,wc.status);
                // if(!ret)
                //     continue;
                srmc->sig_cnt--;
                if(wc.status!=IB_WC_SUCCESS){
                    continue;
                }
                cqe64 =(to_mcq(srmc->init_qp->ibqp.send_cq)->mcq.cqe_sz == 64) ? cqe : cqe + 64;
                //Two attr to change
                

                qpn = (wc.wr_id>>32) & 0xffffff;//wr_id high 32 bits is qpn,low  32 bits is wqe_counter
                pr_info("wc's qpn:%d,wc's wqe_counter:%d\n",qpn,wc.wr_id & 0xffffffff);
                mutex_lock(&sched->sq_lock);
                for(sqb=sched->sq_head;sqb;sqb=sqb->next){
                    if(sqb->qpn == qpn){
                        cqn = sqb->cqn;
                        break;
                    }
                }
                mutex_unlock(&sched->sq_lock);
                if(cqn == -1){
                    pr_err("Unexpected:No cqn found for qpn %d\n",qpn);
                    break;
                }
                mutex_lock(&sched->cq_lock);
                for(cqb=sched->cq_head;cqb;cqb=cqb->next){
                    if(cqb->cqn == cqn){
                        //distribute
                        //TODO:change the owner bit
                        pr_info("find cqn:%d\n",cqn);
                        ucqe = cqb->buf + cqb->cur_put * cqb->cqe_sz;
                        ucqe64 = (cqb->cqe_sz == 64) ? ucqe : ucqe + 64;
                        memcpy(ucqe,cqe,cqb->cqe_sz);
                        ucqe64->sop_drop_qpn = htonl(ntohl(ucqe64->sop_drop_qpn)-srmc->init_qp->ibqp.qp_num + qpn);
                        ucqe64->wqe_counter =  htonl(wc.wr_id & 0xffffffff);
                        cqb->cur_put ++ ;
                        if(cqb->cur_put* cqb->cqe_sz >= cqb->cq_size){
                            cqb->cur_put = 0;
                        }
                        break;
                    }
                }
                mutex_unlock(&sched->cq_lock);
            }
            pr_info("polling cqe\n");
        }
        mutex_unlock(&sched->srmc_lock);
        msleep(10000);
    }
    pr_info("scheduler thread exit\n");
    pr_info("kthread_should_stop: %d\n", kthread_should_stop());
	return 0;
}

//create srmc, and copy the qp struct to it.
int mlx5_ib_create_srmc(struct mlx5_ib_sched *sched,struct mlx5_ib_qp *init_qp,struct mlx5_ib_qp *tgt_qp,union ib_gid *dgid)
{
    pr_info("in mlx5_ib_create_srmc,gid.in_id = %llu, gid.subnet = %llu\n",dgid->global.interface_id,dgid->global.subnet_prefix);
    struct mlx5_ib_srmc *srmc;
    int ret;
    int find;
    find = 0;
    if(init_qp == NULL && tgt_qp == NULL){
        pr_err("Unexpected:Both qp are NULL\n");
        return -1;
    }
    mutex_lock(&sched->srmc_lock);
    for(srmc=sched->srmc_head;srmc;srmc=srmc->next){
        if(memcmp(srmc->dgid.raw,dgid->raw,sizeof(srmc->dgid.raw))==0){
            find = 1;
            break;
        }
    }
    if(!find){
        pr_err("Unexpected:No srmc found in creating srmc function\n");
        mutex_unlock(&sched->srmc_lock);
        return -1;
    }

    if(init_qp){
        //TODO:create init qp
        if(srmc->init_qp == NULL){
            srmc->init_qp = init_qp;
        }else{
            pr_err("Unexpected:Init qp already exists\n");
            goto err;
        }
    }
    if(tgt_qp){
        //TODO:create tgt qp
        if(srmc->tgt_qp == NULL){
            srmc->tgt_qp = tgt_qp;
        }else{  
            pr_err("Unexpected:Tgt qp already exists\n");
            goto err;
        }
    }
    mutex_unlock(&sched->srmc_lock);
	return 0;

err:
    mutex_unlock(&sched->srmc_lock);
    return -1;
}

int mlx5_ib_sched_init(struct mlx5_ib_sched* sched)
{
    sched->task = kthread_create(scheduler_polling,(void*)sched,"polling thread");//void* can accept any type of pointer
	sched->sq_head = NULL;
    sched->cq_head = NULL;
    sched->srmc_head = NULL;
	mutex_init(&sched->sq_lock);
    mutex_init(&sched->cq_lock);
    mutex_init(&sched->srmc_lock);
	if (IS_ERR(sched->task)) { 
		pr_err("Failed to create polling thread\n"); 
		return PTR_ERR(sched->task);
	}
	int cpu_id = 0;
	kthread_bind(sched->task, cpu_id);
	wake_up_process(sched->task);
	pr_info("Polling thread started and bound to CPU %d\n",cpu_id);

    return 0;
}

void mlx5_ib_sched_exit(struct mlx5_ib_sched* sched)
{
    struct mlx5_ib_sqbuf *sqb;
    struct mlx5_ib_cqbuf *cqb;
    struct mlx5_ib_srmc *srmc;
    int npages;
    int i;
    //rmmod will call mlx5_ib_cleanup, and then call mlx5_ib_sched_exit, so we need to check if sched->task is NULL or not to avoid blocked
    pr_info("Ready to stop sched->task\n");
    if (sched->task) {
        kthread_stop(sched->task); 
        pr_info("Cleaning polling thread resources\n");
        sched->task = NULL;
    } else {
        pr_info("Scheduler task is not running\n");
    }
    //cleanup scheduler
    for(sqb=sched->sq_head;sqb;){
        vunmap(sqb->buf);
        npages = (sqb->sq_size +PAGE_SIZE-1)/PAGE_SIZE;
        for(i=0;i<npages;i++)
            put_page(sqb->pages[i]);
        kfree(sqb->pages);


        sqb = sqb->next;
        kfree(sched->sq_head);
        sched->sq_head = sqb;
    }
    pr_info("clean sqb success\n");
    for(cqb=sched->cq_head;cqb;){
        vunmap(cqb->buf);
        npages = (cqb->cq_size +PAGE_SIZE-1)/PAGE_SIZE;
        for(i=0;i<npages;i++)
            put_page(cqb->pages[i]);
        kfree(cqb->pages);
        cqb = cqb->next;
        kfree(sched->cq_head);
        sched->cq_head = cqb;
    }
    pr_info("clean cqb success\n");
    //TODO:cleanup srmc
	for(srmc = sched->srmc_head;srmc;){
        
        srmc = srmc->next;
        kfree(sched->srmc_head);
        sched->srmc_head = srmc;
    }
    pr_info("clean srmc success\n");
    mlx5_ib_unmap_ubuf(sched,0);
}

//return 0 means xrc exists, other means xrc not exists
int is_xrc_exists(struct mlx5_ib_sched* sched,struct ib_pd *pd,union ib_gid *dgid,int flags,int qpn){
    
    pr_info("in is_xrc_exists,gid.in_id = %llx, gid.subnet = %llx\n",dgid->global.interface_id,dgid->global.subnet_prefix);
    struct mlx5_ib_srmc *srmc;
    int ret = 1;
    mutex_lock(&sched->srmc_lock);
    for(srmc=sched->srmc_head;srmc;srmc=srmc->next){
        if(memcmp(srmc->dgid.raw,dgid->raw,sizeof(srmc->dgid.raw))==0){
            if(flags == SRMC_CREATE_FLAG_INIT_QP){
                if(srmc->ini_refcnt == 0){
                    srmc->ini_refcnt = 1;
                    ret = 1;
                }else{
                    srmc->ini_refcnt++;
                    ret = 0;
                }
            }else if(flags == SRMC_CREATE_FLAG_TGT_QP){
                if(srmc->tgt_refcnt == 0){
                    srmc->tgt_refcnt = 1;
                    ret = 1;
                }else{
                    srmc->tgt_refcnt++;
                    ret = 0;
                }
            }
            break;
        }
    }
    if(srmc == NULL){
        //srmc no exists
        if(sched->srmc_head == NULL){
            srmc = sched->srmc_head = kzalloc(sizeof(struct mlx5_ib_srmc),GFP_KERNEL);
        }else{
            srmc = kzalloc(sizeof(struct mlx5_ib_srmc),GFP_KERNEL);
            srmc->next = sched->srmc_head->next;
            sched->srmc_head->next = srmc;
        }
        memcpy(srmc->dgid.raw,dgid->raw,sizeof(srmc->dgid.raw));
        if(flags == SRMC_CREATE_FLAG_INIT_QP){
            srmc->ini_refcnt = 1;
         }else{
            srmc->tgt_refcnt = 1;
         }
    }
    if(ret&&flags == SRMC_CREATE_FLAG_INIT_QP){
        if((ret = mlx5_ib_create_srmc_qp(sched,srmc,pd,flags,qpn))<0){
            pr_err("Failed to create srmc qp\n");
        }
    }
    mutex_unlock(&sched->srmc_lock);
    pr_info("out is_xrc_exists,ret:%d\n",ret);
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

int mlx5_ib_destroy_srmc(struct mlx5_ib_sched* sched,int ah_id){
    struct mlx5_ib_srmc *srmc,*tmp;
    mutex_lock(&sched->srmc_lock);
    if(sched->srmc_head==NULL){
        goto err;
    }
    if(sched->srmc_head->ah_id == ah_id){
        srmc = sched->srmc_head;
        sched->srmc_head = srmc->next;
    }else{
        for(srmc=sched->srmc_head;srmc->next;srmc=srmc->next){
            if(srmc->next->ah_id == ah_id){
                break;
            }
        }
        if(srmc->next==NULL)
            goto err;
        tmp = srmc->next;
        srmc->next = tmp->next;
        srmc = tmp;
    }
    //destroy qp
    if(srmc->init_qp)
        ib_destroy_qp_user(&srmc->init_qp->ibqp,NULL);
    if(srmc->tgt_qp)
        ib_destroy_qp_user(&srmc->tgt_qp->ibqp,NULL);

    kfree(srmc);
    //success
    mutex_unlock(&sched->srmc_lock);
    return 0;
err:
    mutex_unlock(&sched->srmc_lock);
    return -1;
}

static void print_pd_info(struct ib_pd *pd)
{
    if (!pd) {
        pr_err("pd 指针为 NULL\n");
        return;
    }

    // 输出 local_dma_lkey 和 unsafe_global_rkey
    pr_info("SRMC pd->local_dma_lkey: %u\n", pd->local_dma_lkey);
    pr_info("SRMC pd->unsafe_global_rkey: %u\n", pd->unsafe_global_rkey);

    // 检查并输出 __internal_mr 的地址
    if (pd->__internal_mr) {
        pr_info("pd->__internal_mr 地址: %pK\n", pd->__internal_mr);
        pr_info("pd->__internal_mr->lkey: %u\n", pd->__internal_mr->lkey);
        pr_info("pd->__internal_mr->rkey: %u\n", pd->__internal_mr->rkey);
        
        // 如果需要更多 MR 信息，可以继续添加
        pr_info("pd->__internal_mr->iova: 0x%llx\n", pd->__internal_mr->iova);
        pr_info("pd->__internal_mr->length: 0x%llx\n", pd->__internal_mr->length);
    } else {
        pr_info("SRMC pd->__internal_mr 为 NULL\n");
    }
}

//return 0 if success, return -1 if failed. return >0 for qpn.
int mlx5_ib_create_srmc_qp(struct mlx5_ib_sched* sched,struct mlx5_ib_srmc *srmc,struct ib_pd *pd,int flags,int qpn){
    struct ib_qp *qp;
    struct ib_qp_init_attr create_attr;
    struct ib_cq *cq;
    struct ib_cq_init_attr cq_attr;
    struct ib_xrcd *xrcd;
    struct ibv_qp_info local_qp_info,remote_qp_info;
    memset(&local_qp_info,0,sizeof(local_qp_info));
    memset(&remote_qp_info,0,sizeof(remote_qp_info));
    memset(&cq_attr,0,sizeof cq_attr);
    const int sq_depth = 256;
    const int cq_depth = 256;
    const int rq_depth = 256;
    const int psn = 3100;
    const int max_rd_atomic = 16;
    const int port_num = 1;
    memset(&create_attr,0,sizeof create_attr);
    struct ib_qp_attr conn_attr;
    int ret;
    // struct ib_device *ibdev;
    print_pd_info(pd);
    



    cq_attr.cqe = cq_depth;
    cq_attr.comp_vector = 0;// CPU 0
    cq = ib_create_cq(pd->device,NULL,NULL,NULL,&cq_attr);
    if(!cq){
        pr_err("Create CQ false\n");
        return -1;
    }
    if(flags == SRMC_CREATE_FLAG_INIT_QP){
        if(srmc->init_qp){
            pr_err("Unexpected:init qp exists\n");
            return -1;
        }
        // //alloc mr
        // srmc->ini_mr = ib_alloc_mr(pd,IB_MR_TYPE_MEM_REG,1);
        // if(srmc->ini_mr==NULL){
        //     pr_err("Alloc mr false\n");
        //     return -1;
        // }


        create_attr.qp_type = IB_QPT_XRC_INI;
        create_attr.send_cq = cq;


        // create_attr.qp_type = IB_QPT_RC;
        // create_attr.recv_cq = cq;//RC
        // create_attr.cap.max_recv_wr = 1;
        // create_attr.cap.max_recv_sge = 1;//RC

        create_attr.cap.max_send_wr = sq_depth;
        create_attr.cap.max_send_sge = 1;
        create_attr.cap.max_inline_data = 128;
        // create_attr.cap.max_rdma_ctxs = 1;
        create_attr.sq_sig_type = IB_SIGNAL_REQ_WR;
        pr_info("ready to ib_create_qp_kernel\n");
        qp = ib_create_qp_kernel(pd,&create_attr,"scheduler");
        pr_info("flags == SRMC_CREATE_FLAG_INIT_QP create qp_kernel\n");
        if(!qp){
            pr_err("Create Kernel qp false\n");
            return -1;
        }
        local_qp_info.qpn = qp->qp_num;
        // 打印 subnet_prefix 和 interface_id（需注意大端序转换）
        printk(KERN_INFO "Subnet Prefix: %llx\n", srmc->dgid.global.subnet_prefix);
        printk(KERN_INFO "Interface ID: %llx\n", srmc->dgid.global.interface_id);
        // memcpy(local_qp_info.gid.raw,srmc->dgid.raw,sizeof(union ib_gid));//now just the same server,TODO:change to local gid.
        printk(KERN_INFO "local_qp_info GID (raw): %u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u\n",
            local_qp_info.gid.raw[0], local_qp_info.gid.raw[1], local_qp_info.gid.raw[2], local_qp_info.gid.raw[3],
            local_qp_info.gid.raw[4], local_qp_info.gid.raw[5], local_qp_info.gid.raw[6], local_qp_info.gid.raw[7],
            local_qp_info.gid.raw[8], local_qp_info.gid.raw[9], local_qp_info.gid.raw[10], local_qp_info.gid.raw[11],
            local_qp_info.gid.raw[12], local_qp_info.gid.raw[13], local_qp_info.gid.raw[14], local_qp_info.gid.raw[15]);
        
        // ibdev = ib_device_get_by_name("mlx5_0", RDMA_DRIVER_MLX5);//内核打开设备，类似open_device
        // 打印 ibdev->name 以确认设备名称
        pr_info("Successfully got IB device: %s\n", pd->device->name);
        // ret= ibdev->ops.query_gid(ibdev, port_num, 0, &local_qp_info.gid);//不适用于Rocev2
        // ret = pd->device->ops.query_gid(pd->device,port_num,0,&local_qp_info.gid);
        ret=rdma_query_gid(pd->device,port_num,0,&local_qp_info.gid);
        if(ret){
            pr_err("local query_gid false,error code:%d\n",ret);
            return -1;
        }
        pr_info("local gid interface_id: 0x%llx, subnet_prefix:0x%llx\n",local_qp_info.gid.global.interface_id,local_qp_info.gid.global.subnet_prefix);
        memset(&remote_qp_info,0,sizeof(remote_qp_info));
        
        mutex_unlock(&sched->srmc_lock);//give the lock to server
        int rtry_cnt = 0;
        while(conn_server_kernel("127.0.0.1",12345,&local_qp_info,&remote_qp_info) < 0){
            pr_err("conn_server_kernel failed\n");
            rtry_cnt++;
            if(rtry_cnt > 10){
                pr_err("conn_server_kernel failed %d times\n",rtry_cnt);
                mutex_lock(&sched->srmc_lock);
                return -1;
            }
        }
        mutex_lock(&sched->srmc_lock);

        srmc->init_qp = to_mqp(qp);
        srmc->init_cq = to_mcq(cq);
    }else{
        if(srmc->tgt_qp){
            pr_err("Unexpected:tgt qp exists\n");
            return -1;
        }
        // //alloc mr
        // srmc->tgt_mr = ib_alloc_mr(pd,IB_MR_TYPE_MEM_REG,1);
        // if(srmc->tgt_mr==NULL){
        //     pr_err("Alloc mr false\n");
        //     return -1;
        // }


        //为了统一modify操作赋值remote qp info
        remote_qp_info.qpn = qpn;
        remote_qp_info.gid.global.interface_id = srmc->dgid.global.interface_id;   
        remote_qp_info.gid.global.subnet_prefix = srmc->dgid.global.subnet_prefix;


        create_attr.qp_type = IB_QPT_XRC_TGT;
        create_attr.xrcd = xrcd;
    
        // create_attr.send_cq = cq;
        // create_attr.qp_type = IB_QPT_RC;
        // create_attr.cap.max_send_sge = 1;
        // create_attr.cap.max_send_wr = sq_depth;//RC
        // create_attr.cap.max_rdma_ctxs = 1;
        // create_attr.comp_mask = IBV_QP_INIT_ATTR_PD;
        // create_attr.pd = pd;
    
        create_attr.recv_cq = cq;
        create_attr.cap.max_recv_wr = rq_depth;  
        create_attr.cap.max_recv_sge = 1;
        create_attr.cap.max_inline_data = 128;
        pr_info("ready to ib_create_qp_kernel else\n");
        qp = ib_create_qp_kernel(pd,&create_attr,"scheduler");
        if(!qp){
            pr_err("Create Kernel qp false\n");
            return -1;
        }
        pr_info("flags != SRMC_CREATE_FLAG_INIT_QP create qp_kernel\n");
        srmc->tgt_qp = to_mqp(qp);
    }
    //modify qp
    memset(&conn_attr,0,sizeof conn_attr);
    conn_attr.qp_state = IB_QPS_INIT;
    conn_attr.pkey_index = 0;
    conn_attr.port_num = port_num;//is that ok?
    conn_attr.qp_access_flags = IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ | IB_ACCESS_REMOTE_ATOMIC;
    if (ib_modify_qp(qp, &conn_attr,
            IB_QP_STATE | IB_QP_PKEY_INDEX | IB_QP_PORT |
                IB_QP_ACCESS_FLAGS)) {
        pr_err("Failed to modify conn QP to INIT\n");
        return -1;
    }

    memset(&conn_attr, 0, sizeof(struct ib_qp_attr));
    conn_attr.qp_state = IB_QPS_RTR;
    conn_attr.path_mtu = IB_MTU_1024;
    conn_attr.dest_qp_num = remote_qp_info.qpn;
    pr_info("remote qpn: %d\n",remote_qp_info.qpn);
    conn_attr.rq_psn = psn;

    //conn_attr.ah_attr.is_global = 1; RoCE or IB???
    conn_attr.ah_attr.ib.dlid = 0;
    conn_attr.ah_attr.sl = 0;
    conn_attr.ah_attr.ib.src_path_bits = 0;
    conn_attr.ah_attr.port_num = 1;  // Local port?is that ok?


    struct ib_global_route *grh = &conn_attr.ah_attr.grh;
    grh->dgid.global.interface_id = remote_qp_info.gid.global.interface_id;
    grh->dgid.global.subnet_prefix = remote_qp_info.gid.global.subnet_prefix;
    pr_info("remote gid interface_id: %llx, subnet_prefix:%llx\n",remote_qp_info.gid.global.interface_id,remote_qp_info.gid.global.subnet_prefix);
    grh->sgid_index = 0;
    grh->hop_limit = 1;

    conn_attr.ah_attr.type = RDMA_AH_ATTR_TYPE_ROCE;
    conn_attr.ah_attr.ah_flags = IB_AH_GRH;
    

    int rtr_flags = IB_QP_STATE | IB_QP_AV | IB_QP_PATH_MTU | IB_QP_DEST_QPN |
                    IB_QP_RQ_PSN;

    if(flags == SRMC_CREATE_FLAG_TGT_QP){
        conn_attr.max_dest_rd_atomic = max_rd_atomic;
        conn_attr.min_rnr_timer = 12;
        rtr_flags |= IB_QP_MAX_DEST_RD_ATOMIC | IB_QP_MIN_RNR_TIMER;
    }

    if (ib_modify_qp(qp, &conn_attr, rtr_flags)) {
        pr_err("Failed to modify QP to RTR\n");
        return -1;
    }

    memset(&conn_attr, 0, sizeof(conn_attr));
    conn_attr.qp_state = IB_QPS_RTS;
    conn_attr.sq_psn = psn;

    int rts_flags = IB_QP_STATE | IB_QP_SQ_PSN;

    conn_attr.timeout = 60;
    conn_attr.retry_cnt = 30;//增大重试时间
    conn_attr.rnr_retry = 30;
    conn_attr.max_rd_atomic = max_rd_atomic;
    conn_attr.max_dest_rd_atomic = max_rd_atomic;
    rts_flags |= IB_QP_TIMEOUT | IB_QP_RETRY_CNT | IB_QP_RNR_RETRY |
                IB_QP_MAX_QP_RD_ATOMIC;
    
    
    if (ib_modify_qp(qp, &conn_attr, rts_flags)) {
        pr_err("Failed to modify QP to RTS\n");
        return -1;
    }
    pr_info("Successfully created SRMC\n");
    if(flags == SRMC_CREATE_FLAG_INIT_QP)
        return 0;
    else 
        return qp->qp_num;
}