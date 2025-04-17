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
#include <linux/err.h>
#include "user_verbs.h"
#include "conn.h"
#include <linux/inet.h>
#include <linux/jhash.h>
#include <rdma/ib_cm.h>

#define IP_ADDR "192.168.1.5"
int mlx5_ib_map_ubuf(struct mlx5_ib_sched_group* sched_group,unsigned long virt_addr,size_t size,int qpn,int cqn){
    DEBUG_LOG("in mlx5_ib_map_ubuf\n");
    DEBUG_LOG("内核态virt_addr:%px,size:%d,qpn:%d,cqn%d\n",virt_addr,size,qpn,cqn);
    //目前srm qp 的wqe大小为128
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
    mutex_lock(&sched_group->sq_lock);
    uq = kzalloc(sizeof(struct mlx5_ib_sqbuf),GFP_KERNEL);
    uq->cqn = cqn;
    uq->qpn = qpn;
    uq->sq_size = size;
    uq->buf = vmap(pages,npages,VM_MAP,PAGE_KERNEL);//map the pages(phys addr) to kernel space（virtual addr），非连续物理页映射到虚拟页
    uq->pages = pages;
    if(!uq->buf){
        for (i = 0; i < ret; i++)
            put_page(pages[i]);
        kfree(pages);
        kfree(uq);
        pr_err("fail to map sq buffer\n");
        mutex_unlock(&sched_group->sq_lock);
        return -ENOMEM;
    }
    if(sched_group->sq_head == NULL){
        sched_group->sq_head = uq;
    }else{   
        uq->next = sched_group->sq_head->next;
        sched_group->sq_head->next = uq;
    }
    DEBUG_LOG("map sq buf success\n");
    mutex_unlock(&sched_group->sq_lock);

    return 0;
}

int mlx5_ib_map_cq_ubuf(struct mlx5_ib_sched_group* sched_group,unsigned long virt_addr,size_t size,int cqn){
    DEBUG_LOG("in mlx5_ib_map_cq_ubuf\n");
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
    mutex_lock(&sched_group->cq_lock);
    uq =  kzalloc(sizeof(struct mlx5_ib_cqbuf),GFP_KERNEL);
    uq->cqn = cqn;
    uq->cq_size = size;
    uq->buf = vmap(pages,npages,VM_MAP,PAGE_KERNEL);
    uq->pages = pages;
    uq->cqe_sz = 64;
    if(!uq->buf){
        kfree(uq);
        for (i = 0; i < ret; i++)
            put_page(pages[i]);
        kfree(pages);
        pr_err("failed to map cf buffer\n");
        mutex_unlock(&sched_group->cq_lock);
        return -ENOMEM;
    }
    if(sched_group->cq_head == NULL){
        sched_group->cq_head = uq;
    }else{
        uq->next = sched_group->cq_head->next;
        sched_group->cq_head->next = uq;
    }
    mutex_unlock(&sched_group->cq_lock);

    return 0;
}

int mlx5_ib_unmap_ubuf(struct mlx5_ib_sched_group* sched_group,int qpn){
    int cqn;
    struct mlx5_ib_sqbuf *sqb,*tmp;
    struct mlx5_ib_cqbuf *cqb,*tmp2;
    int npages;
    int i;
    mutex_lock(&sched_group->sq_lock);
    pr_info("mlx5_ib_unmap_ubuf清除映射资源\n");
    if(sched_group->sq_head==NULL){
        mutex_unlock(&sched_group->sq_lock);
        return 0;
    }
    if(sched_group->sq_head->qpn == qpn){
        cqn = sched_group->sq_head->cqn;
        sqb = sched_group->sq_head;
        sched_group->sq_head = sqb->next;
        vunmap(sqb->buf);
        npages = (sqb->sq_size +PAGE_SIZE-1)/PAGE_SIZE;
        for(i=0;i<npages;i++)
            put_page(sqb->pages[i]);
        kfree(sqb->pages);
        kfree(sqb);
    }
    else{
        for(sqb=sched_group->sq_head->next;sqb->next;sqb=sqb->next){
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
    mutex_unlock(&sched_group->sq_lock);

    //Free cq
    mutex_lock(&sched_group->cq_lock);
    if(sched_group->cq_head==NULL){
        mutex_unlock(&sched_group->cq_lock);
        return 0;
    }
    if(sched_group->cq_head->cqn == cqn){
        cqb = sched_group->cq_head;
        sched_group->cq_head = cqb->next;
        vunmap(cqb->buf);
        npages = (cqb->cq_size +PAGE_SIZE-1)/PAGE_SIZE;
        for(i=0;i<npages;i++)
            put_page(cqb->pages[i]);
        kfree(cqb->pages);
        kfree(cqb);
    }
    else{
        for(cqb=sched_group->cq_head->next;cqb->next;cqb=cqb->next){
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
    mutex_unlock(&sched_group->cq_lock);
    return 0;

}
int scheduler_polling(void* data)
{
    extern struct mlx5_ib_sched_group sched_group;
    int ret;
    struct mlx5_ib_sched_id *sched_id = (struct mlx5_ib_sched_id*) data;
    struct mlx5_ib_sched* sched = sched_id->sched;
    int id = sched_id->id;
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
    int op_own;

    kfree(sched_id);
    
    while(!kthread_should_stop()){
        
        // pr_info("polling hhhhh\n");
        //mutex_lock(&sched_group.sq_lock);
        for(sqb = sched_group.sq_head;sqb;sqb=sqb->next){//是否应该加一个检查该QP有几个WR，都拉下来的逻辑？
            ibv_wr = (struct ibv_send_wr*)(sqb->buf + sqb->cur_post * sizeof(struct ibv_send_wr));
            if(!ibv_wr->wr_id || sched_hash_gid(&ibv_wr->qp_type.srm.remote_gid,sched_group.num_sched) != id){
                continue;
            }
            pr_info("posting wr,sizeof wr is %u\n",sizeof(struct ibv_send_wr));
            pr_info("qpn:%d,cur_post:%d,wr_id:%llu,sq buffer size:%d\n",sqb->qpn,sqb->cur_post,ibv_wr->wr_id,sqb->sq_size);

            memset(&rdma_wr,0,sizeof rdma_wr);


            //can just have one wr for now 
            rdma_wr.wr.wr_id = (((uint64_t)sqb->qpn) << 32) | sqb->cur_post;
            DEBUG_LOG("rdma_wr.wr.wr_id:%llu\n",rdma_wr.wr.wr_id);
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
            DEBUG_LOG("发送端的数据缓存区地址: %px\n", sgl.addr);
            DEBUG_LOG("发送端要写入的缓存区地址: %px\n", rdma_wr.remote_addr);
            DEBUG_LOG("服务端lkey：%d，收到的rkey：%d\n", sgl.lkey, rdma_wr.rkey);

            // for(srmc=sched->srmc_head;srmc;srmc=srmc->next){
            //     struct ib_qp_attr qp_attr_inpolling2;
            //     struct ib_qp_init_attr qp_init_attr_inpolling2;
            //     ib_query_qp(&srmc->ini_cb.qp->ibqp,&qp_attr_inpolling2,0,&qp_init_attr_inpolling2);
            //     if(qp_attr_inpolling2.cur_qp_state==3)DEBUG_LOG("in scheduler_polling2, qp cur state:%d\n",qp_attr_inpolling2.cur_qp_state);
            //     else {DEBUG_LOG("in scheduler_polling2, qp cur state:%d\n",qp_attr_inpolling2.cur_qp_state);
            //         DEBUG_LOG("msleep2 100ms\n");msleep(100);}
            // }

            //find the srmc qp to send this req
            //mutex_lock(&sched->srmc_lock);
            for(srmc=sched->srmc_head;srmc;srmc=srmc->next){
                //DEBUG_LOG("found srmc,gid.interface_id:%llx,subnet_prefix:%llx\n",srmc->dgid.global.interface_id,srmc->dgid.global.subnet_prefix);
                if(memcmp(srmc->dgid.raw,ibv_wr->qp_type.srm.remote_gid.raw,sizeof(srmc->dgid.raw))==0){
                    if(!srmc->ini_cb.qp){
                        pr_err("Unexpected:ini qp for this srmc is NULL\n");
                        break;
                    }
                    DEBUG_LOG("max_gs:%d\n,max_post:%d,fbc:%u\n",srmc->ini_cb.qp->sq.max_gs,srmc->ini_cb.qp->sq.max_post,srmc->ini_cb.qp->sq.fbc);
                    if(ret = ib_post_send(&srmc->ini_cb.qp->ibqp,&rdma_wr.wr,&bad_wr)){
                        pr_err("post send error:%d\n",ret);
                    }
                    DEBUG_LOG("send finished\n");
                    srmc->ini_cb.sig_cnt+=(rdma_wr.wr.send_flags & IB_SEND_SIGNALED) ? 1 : 0;
                    break;
                }
            }
           // mutex_unlock(&sched->srmc_lock);
            if(sqb->cur_post*sizeof(struct ibv_send_wr) >= sqb->sq_size){
                sqb->cur_post = 0;
            }
            
        }
        //mutex_unlock(&sched_group.sq_lock);  
        //poll xrc cq and distribute them
        //mutex_lock(&sched->srmc_lock);
        for(srmc=sched->srmc_head;srmc;srmc=srmc->next){
            if(srmc->ini_cb.sig_cnt){
                DEBUG_LOG("distributing cqe\n");
                memset(&wc,1,sizeof wc);
                int cqe_num=0;
                while(!cqe_num){
                    //cqe_num=ib_poll_cq(srmc->ini_cb.qp->ibqp.send_cq,1,&wc);
                    cqe_num = mlx5_ib_poll_cq_with_cqe(srmc->ini_cb.qp->ibqp.send_cq,1,&wc,&cqe);
                }
                DEBUG_LOG("cqe_num:%d,wc status:%d\n",cqe_num,wc.status);
                srmc->ini_cb.sig_cnt--;
                cqe64 =(to_mcq(srmc->ini_cb.qp->ibqp.send_cq)->mcq.cqe_sz == 64) ? cqe : cqe + 64;
                //Two attr to change
                

                qpn = (wc.wr_id>>32) & 0xffffff;//wr_id high 32 bits is qpn,low  32 bits is wqe_counter
                pr_info("wc's qpn:%d,wc's wqe_counter:%d\n",qpn,wc.wr_id & 0xffffffff);
                //mutex_lock(&sched_group.sq_lock);
                for(sqb=sched_group.sq_head;sqb;sqb=sqb->next){
                    if(sqb->qpn == qpn){
                        cqn = sqb->cqn;
                        break;
                    }
                }
               // mutex_unlock(&sched_group.sq_lock);
                if(cqn == -1){
                    pr_err("Unexpected:No cqn found for qpn %d\n",qpn);
                    break;
                }
                //mutex_lock(&sched_group.cq_lock);
                for(cqb=sched_group.cq_head;cqb;cqb=cqb->next){
                    if(cqb->cqn == cqn){
                        //distribute
                        //TODO:change the owner bit
                        DEBUG_LOG("find cqn:%d\n",cqn);
                        ucqe = cqb->buf + cqb->cur_put * cqb->cqe_sz;
                        ucqe64 = (cqb->cqe_sz == 64) ? ucqe : ucqe + 64;
                        memcpy(ucqe,cqe,cqb->cqe_sz);
                        DEBUG_LOG("cqe64->op_own:%x,cqe_size:%d\n",ucqe64->op_own,cqb->cqe_sz);
                        ucqe64->sop_drop_qpn = htonl(ntohl(ucqe64->sop_drop_qpn)&(~0xffffff) | qpn);
                        ucqe64->wqe_counter =  htons(wc.wr_id & 0xffff);
                        //反转用户态cqe的owner_bit
                        ucqe64->op_own = (ucqe64->op_own & (~0xf)) | cqb->op_own;
                        DEBUG_LOG("ucqe64->op_own:%x,op_own:%d\n",ucqe64->op_own,op_own);
                        cqb->cur_put ++ ;
                        if(cqb->cur_put* cqb->cqe_sz >= cqb->cq_size){
                            cqb->cur_put = 0;
                            cqb->op_own ^= MLX5_CQE_OWNER_MASK;
                        }
                        DEBUG_LOG("distribute cqe finished\n");
                        break;
                    }
                }
                //mutex_unlock(&sched_group.cq_lock);
            }
            //pr_info("polling cqe\n");
        }
        //mutex_unlock(&sched->srmc_lock);
        msleep(100);
    }
    DEBUG_LOG("scheduler thread exit\n");
	return 0;
}

// //create srmc, and copy the qp struct to it.
// int mlx5_ib_create_srmc(struct mlx5_ib_sched *sched,struct mlx5_ib_qp *init_qp,struct mlx5_ib_qp *tgt_qp,union ib_gid *dgid)
// {
//     pr_info("in mlx5_ib_create_srmc,gid.in_id = %llu, gid.subnet = %llu\n",dgid->global.interface_id,dgid->global.subnet_prefix);
//     struct mlx5_ib_srmc *srmc;
//     int ret;
//     int find;
//     find = 0;
//     if(init_qp == NULL && tgt_qp == NULL){
//         pr_err("Unexpected:Both qp are NULL\n");
//         return -1;
//     }
//     mutex_lock(&sched->srmc_lock);
//     for(srmc=sched->srmc_head;srmc;srmc=srmc->next){
//         if(memcmp(srmc->dgid.raw,dgid->raw,sizeof(srmc->dgid.raw))==0){
//             find = 1;
//             break;
//         }
//     }
//     if(!find){
//         pr_err("Unexpected:No srmc found in creating srmc function\n");
//         mutex_unlock(&sched->srmc_lock);
//         return -1;
//     }

//     if(init_qp){
//         //TODO:create init qp
//         if(srmc->init_qp == NULL){
//             srmc->init_qp = init_qp;
//         }else{
//             pr_err("Unexpected:Init qp already exists\n");
//             goto err;
//         }
//     }
//     if(tgt_qp){
//         //TODO:create tgt qp
//         if(srmc->tgt_qp == NULL){
//             srmc->tgt_qp = tgt_qp;
//         }else{  
//             pr_err("Unexpected:Tgt qp already exists\n");
//             goto err;
//         }
//     }
//     mutex_unlock(&sched->srmc_lock);
// 	return 0;

// err:
//     mutex_unlock(&sched->srmc_lock);
//     return -1;
// }

int mlx5_ib_sched_init(struct mlx5_ib_sched_group* sched_group,int num)
{
    int i;
    int ret;
    struct mlx5_ib_sched_id *sched_id; 

    sched_group->sq_head = NULL;
    sched_group->cq_head = NULL;
	mutex_init(&sched_group->sq_lock);
    mutex_init(&sched_group->cq_lock);


    sched_group->num_sched = num;
    sched_group->scheds = kzalloc(num*sizeof(struct mlx5_ib_sched),GFP_KERNEL);
    char thread_info[64];
    for(i=0;i<num;i++){
        sched_group->scheds[i].srmc_head = NULL;

        snprintf(thread_info, sizeof(thread_info), "sched_thread_%d", i);
        
        sched_id = kzalloc(sizeof(struct mlx5_ib_sched_id),GFP_KERNEL);
        sched_id->sched = &sched_group->scheds[i];
        sched_id->id = i;
        sched_group->scheds[i].task = kthread_create(scheduler_polling,(void*)sched_id,thread_info);
        mutex_init(&sched_group->scheds[i].srmc_lock);
        
        if (IS_ERR(sched_group->scheds[i].task)) { 
            pr_err("Failed to create polling thread%d\n",i); 
            ret = PTR_ERR(sched_group->scheds[i].task);
            goto err;
        }
        kthread_bind(sched_group->scheds[i].task, i);
        wake_up_process(sched_group->scheds[i].task);
        pr_info("Polling thread %d started and bound to CPU %d\n",i,i);
    }
    return 0;
err:
    for(i=0;i<num;i++){
        if(sched_group->scheds[i].task){
            kthread_stop(sched_group->scheds[i].task);
            sched_group->scheds[i].task = NULL;
        }
    }
    kfree(sched_group->scheds);
    return ret;
}

void mlx5_ib_sched_exit(struct mlx5_ib_sched_group* sched_group)
{
    struct mlx5_ib_sqbuf *sqb;
    struct mlx5_ib_cqbuf *cqb;
    struct mlx5_ib_srmc *srmc;
    struct mlx5_ib_sched *sched;
    int npages;
    int i;
    for(i=0;i<sched_group->num_sched;i++){
        DEBUG_LOG("Ready to stop sched->task %d\n",i);
        sched = &sched_group->scheds[i];
        if (sched->task) {
            kthread_stop(sched->task); 
            sched->task = NULL;
        }

        for(srmc = sched->srmc_head;srmc;){
            if(srmc->ini_cb.state == CONNECTED){
                rdma_disconnect(srmc->ini_cb.cm_id);
                //ib_sched_free_buf(&srmc->ini_cb);
                ib_destroy_qp(&srmc->ini_cb.qp->ibqp);
                ib_destroy_cq(srmc->ini_cb.cq);
                rdma_destroy_id(srmc->ini_cb.cm_id);
            }
            srmc = srmc->next;
            kfree(sched->srmc_head);
            sched->srmc_head = srmc;
        }
        DEBUG_LOG("clean thread %d srmc success\n",i);
    }
    //cleanup scheduler
    for(sqb=sched_group->sq_head;sqb;){
        vunmap(sqb->buf);
        npages = (sqb->sq_size +PAGE_SIZE-1)/PAGE_SIZE;
        for(i=0;i<npages;i++)
            put_page(sqb->pages[i]);
        kfree(sqb->pages);


        sqb = sqb->next;
        kfree(sched_group->sq_head);
        sched_group->sq_head = sqb;
    }
    DEBUG_LOG("clean sqb success\n");
    for(cqb=sched_group->cq_head;cqb;){
        vunmap(cqb->buf);
        npages = (cqb->cq_size +PAGE_SIZE-1)/PAGE_SIZE;
        for(i=0;i<npages;i++)
            put_page(cqb->pages[i]);
        kfree(cqb->pages);
        cqb = cqb->next;
        kfree(sched_group->cq_head);
        sched_group->cq_head = cqb;
    }
    DEBUG_LOG("clean cqb success\n");
	
    // mlx5_ib_unmap_ubuf(sched,0);
}
int mlx5_ib_server_init(struct mlx5_ib_server *server){
    server->task = kthread_run(mlx5_sched_run_server,&server->server_cb, "server thread");
    if(IS_ERR(server->task)){
        DEBUG_LOG("Failed to create server thread\n");
        return PTR_ERR(server->task);
    }
    return 0;
}
void mlx5_ib_server_exit(struct mlx5_ib_server *server,struct mlx5_ib_sched_group *sched_group){
    int i;
	struct mlx5_ib_sched *sched;
	struct mlx5_ib_srmc *srmc;


    if(server->task){
		kthread_stop(server->task);
		for(i=0 ;i<sched_group->num_sched;i++){
			sched = &sched_group->scheds[i];
			for(srmc = sched->srmc_head;srmc=srmc;srmc=srmc->next){
				if(srmc->tgt_cb.refcnt){
					rdma_disconnect(srmc->tgt_cb.cm_id);
					ib_destroy_qp(&srmc->tgt_cb.qp->ibqp);
                	ib_destroy_cq(srmc->tgt_cb.cq);
					rdma_destroy_id(srmc->tgt_cb.cm_id);
				}
			}
		}
	}
	else 
		pr_info("server task PTR is err\n");
}
void mlx5_ib_gid2ip(char addr[4],union ib_gid *gid){
    memcpy(addr,gid->raw+12,4);
}
//return 0 means xrc exists, other means xrc not exists
int is_xrc_exists(struct mlx5_ib_sched* sched,struct ib_pd *pd,union ib_gid *dgid,int flags,int qpn){
    
    pr_info("in is_xrc_exists,gid.in_id = %llx, gid.subnet = %llx\n",dgid->global.interface_id,dgid->global.subnet_prefix);
    struct mlx5_ib_srmc *srmc;
    int ret = 1;
    int srmc_exists = 0;
    mutex_lock(&sched->srmc_lock);
    for(srmc=sched->srmc_head;srmc;srmc=srmc->next){
        if(memcmp(srmc->dgid.raw,dgid->raw,sizeof(srmc->dgid.raw))==0){
            srmc_exists = 1;
            if(flags == SRMC_CREATE_FLAG_INIT_QP){
                if(srmc->ini_cb.refcnt == 0){
                    srmc->ini_cb.refcnt = 1;
                    ret = 1;
                }else{
                    srmc->ini_cb.refcnt++;
                    ret = 0;
                }
            }else if(flags == SRMC_CREATE_FLAG_TGT_QP){
                if(srmc->tgt_cb.refcnt == 0){
                    srmc->tgt_cb.refcnt = 1;
                    ret = 1;
                }else{
                    srmc->tgt_cb.refcnt++;
                    ret = 0;
                }
            }
            break;
        }
    }
    if(srmc == NULL){
        //srmc no exists
        srmc =  kzalloc(sizeof(struct mlx5_ib_srmc),GFP_KERNEL);
        memcpy(srmc->dgid.raw,dgid->raw,sizeof(srmc->dgid.raw));
        if(flags == SRMC_CREATE_FLAG_INIT_QP){
            srmc->ini_cb.refcnt = 1;
         }else{
            srmc->tgt_cb.refcnt = 1;
         }
        
        if(sched->srmc_head == NULL){
            sched->srmc_head = srmc;
        }else{
            srmc->next = sched->srmc_head->next;
            sched->srmc_head->next = srmc;
        }
        
    }
    mutex_unlock(&sched->srmc_lock);
    // if(ret&&flags == SRMC_CREATE_FLAG_INIT_QP){
    // if(ret){
    //     if((ret = mlx5_ib_create_srmc_qp(sched,srmc,pd,flags,qpn))<0){
    //         pr_err("Failed to create srmc qp\n");
    //     }
    // }

    if(ret){
        ret = create_srmc_qp_cm(&srmc->ini_cb,pd,dgid);
        DEBUG_LOG("create_srmc_qp_cm ret:%d\n",ret);
    }
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
    if (!pd) {
        pr_err("pd 指针为 NULL\n");
        return;
    }

    // 输出 local_dma_lkey 和 unsafe_global_rkey
    pr_info("SRMC pd->local_dma_lkey: %u\n", pd->local_dma_lkey);
    pr_info("SRMC pd->unsafe_global_rkey: %u\n", pd->unsafe_global_rkey);

    // 检查并输出 __internal_mr 的地址
    if (pd->__internal_mr) {
        pr_info("pd->__internal_mr 地址: %px\n", pd->__internal_mr);
        pr_info("pd->__internal_mr->lkey: %d\n", pd->__internal_mr->lkey);
        pr_info("pd->__internal_mr->rkey: %d\n", pd->__internal_mr->rkey);
        
        // 如果需要更多 MR 信息，可以继续添加
        pr_info("pd->__internal_mr->iova: 0x%llx\n", pd->__internal_mr->iova);
        pr_info("pd->__internal_mr->length: 0x%llx\n", pd->__internal_mr->length);
    } else {
        pr_info("SRMC pd->__internal_mr 为 NULL\n");
    }
}

//alloc dma buf and alloc mr, for ini qp
int mlx5_sched_alloc_mr(struct srm_cb *cb,struct ib_pd *pd){
    cb->buf_sz = 100;
    cb->buf = kzalloc(cb->buf_sz,GFP_KERNEL);
    if(cb->buf)
        cb->dma_buf = ib_dma_map_single(pd->device,cb->buf,cb->buf_sz,DMA_BIDIRECTIONAL);
    if(!cb->buf || ib_dma_mapping_error(pd->device,cb->dma_buf)){
        pr_err("Failed to allocate dma buffer\n");
        kfree(cb->buf);
        return -1;
    }
    
    dma_unmap_addr_set(cb,dma_mapping,cb->dma_buf);

    //alloc mr
    cb->page_list_len = (((cb->buf_sz-1) & PAGE_MASK)+PAGE_SIZE)>>PAGE_SHIFT;
    cb->mr = ib_alloc_mr(pd,IB_MR_TYPE_MEM_REG,cb->page_list_len);
    if(IS_ERR(cb->mr)){
        pr_err("Failed to allocate mr\n");
        goto err;
    }
    return 0;
err:
    //TODO:error handling,free resources
    return -1;
}  


//reg rkey and mr. for ini qp.
int mlx5_sched_reg_mr(struct ib_mr *mr,struct mlx5_ib_qp *qp,char *dma_buf,size_t buf_sz,int page_list_len){
    struct ib_reg_wr reg_wr = {0};
    struct ib_send_wr *bad_wr;
    int ret;
    struct scatterlist sg = {0};
    if(!mr || !qp){
        pr_err("Unexpected:mr or qp is NULL\n");
        return -1;
    }
    reg_wr.wr.opcode = IB_WR_REG_MR;
    reg_wr.mr = mr;
    reg_wr.access = IB_ACCESS_LOCAL_WRITE | IB_ACCESS_REMOTE_WRITE;

    sg_init_marker(&sg,1);

    ib_update_fast_reg_key(mr,1);
    reg_wr.key = mr->rkey;
    
    sg_dma_address(&sg) = dma_buf;
    sg_dma_len(&sg) = buf_sz;

    ret = ib_map_mr_sg(mr,&sg,1,NULL,PAGE_SIZE);
    BUG_ON(ret<=0 || ret > page_list_len);

    ret = ib_post_send(&qp->ibqp,&reg_wr.wr,&bad_wr);
    if(ret){
        pr_err("Failed to post reg mr\n");
    }
    return ret;
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
        if(srmc->ini_cb.qp){
            pr_err("Unexpected:init qp exists\n");
            return -1;
        }
        // //alloc mr
        // srmc->ini_mr = ib_alloc_mr(pd,IB_MR_TYPE_MEM_REG,1);
        // if(srmc->ini_mr==NULL){
        //     pr_err("Alloc mr false\n");
        //     return -1;
        // }

        //alloc mr 
        if(mlx5_sched_alloc_mr(&srmc->ini_cb,pd)){
            pr_err("mr alloc false");
            return -1;
        }


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

        srmc->ini_cb.qp = to_mqp(qp);
        srmc->ini_cb.cq = to_mcq(cq);
        pr_info("srmc->init_qp is OK\n");
    }else{
        if(srmc->tgt_cb.qp){
            pr_err("Unexpected:tgt qp exists\n");
            return -1;
        }
        // //alloc mr
        // srmc->tgt_mr = ib_alloc_mr(pd,IB_MR_TYPE_MEM_REG,1);
        // if(srmc->tgt_mr==NULL){
        //     pr_err("Alloc mr false\n");
        //     return -1;
        // }

        if(mlx5_sched_alloc_mr(&srmc->tgt_cb,pd)){
            pr_err("mr alloc false");
            return -1;
        }

        //为了统一modify操作赋值remote qp info
        remote_qp_info.qpn = qpn;
        remote_qp_info.gid.global.interface_id = srmc->dgid.global.interface_id;   
        remote_qp_info.gid.global.subnet_prefix = srmc->dgid.global.subnet_prefix;


        create_attr.qp_type = IB_QPT_XRC_TGT;
        create_attr.xrcd = xrcd;
    
        create_attr.send_cq = cq;
        create_attr.qp_type = IB_QPT_RC;
        create_attr.cap.max_send_sge = 1;
        create_attr.cap.max_send_wr = sq_depth;//RC
    
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
        srmc->tgt_cb.qp = to_mqp(qp);
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

    //if(flags == SRMC_CREATE_FLAG_TGT_QP){
        conn_attr.max_dest_rd_atomic = max_rd_atomic;
        conn_attr.min_rnr_timer = 12;
        rtr_flags |= IB_QP_MAX_DEST_RD_ATOMIC | IB_QP_MIN_RNR_TIMER;
    //}

    if (ib_modify_qp(qp, &conn_attr, rtr_flags)) {
        pr_err("Failed to modify QP to RTR\n");
        return -1;
    }

    memset(&conn_attr, 0, sizeof(conn_attr));
    conn_attr.qp_state = IB_QPS_RTS;
    conn_attr.sq_psn = psn;

    int rts_flags = IB_QP_STATE | IB_QP_SQ_PSN;

    conn_attr.timeout = 18;
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

    //reg mr
    if(flags == SRMC_CREATE_FLAG_INIT_QP){
        if(mlx5_sched_reg_mr(srmc->ini_cb.mr,srmc->ini_cb.qp,srmc->ini_cb.dma_buf,srmc->ini_cb.buf_sz,srmc->ini_cb.page_list_len)){
            pr_err("reg mr false\n");
            return -1;
        }
    }else{
       
        if(mlx5_sched_reg_mr(srmc->tgt_cb.mr,srmc->tgt_cb.qp,srmc->tgt_cb.dma_buf,srmc->tgt_cb.buf_sz,srmc->tgt_cb.page_list_len)){
            pr_err("reg mr false\n");
            return -1;
        }
    }

    pr_info("Successfully created SRMC\n");
    return qp->qp_num;
}

int srm_create_connection(struct rdma_cm_id *cm_id){
    extern struct mlx5_ib_sched_group sched_group;
    struct mlx5_ib_srmc *srmc;
    struct mlx5_ib_sched *sched;
    union ib_gid dgid ;
    struct ib_cq_init_attr cq_attr;
    struct ib_qp_init_attr init_attr;
    int idx;
    int no_srmc;
    struct srm_cb *cb, *server_cb;
    int ret; 

    server_cb = (struct srm_cb*)cm_id->context;

    rdma_read_gids(cm_id, NULL,&dgid);
    DEBUG_LOG("in srm_create_connection,cma_id = %d,dgid.interface_id = %llx,dgid.subnet_prefix=%llx\n",cm_id,dgid.global.interface_id,dgid.global.subnet_prefix);
    
    idx = sched_hash_gid(&dgid,sched_group.num_sched);
    DEBUG_LOG("idx=%d\n",idx);
    sched = &sched_group.scheds[idx];

    mutex_lock(&sched->srmc_lock);
    for(srmc = sched->srmc_head;srmc;srmc = srmc->next){
        if(memcmp(srmc->dgid.raw,dgid.raw,sizeof(srmc->dgid.raw))==0){
            break;
        }
    }
    if(srmc&&srmc->tgt_cb.refcnt){
        //已经存在TGT QP
        srmc->tgt_cb.refcnt++;
        rdma_destroy_id(cm_id);
        return 0;
    }
    if(srmc == NULL){
        srmc = kzalloc(sizeof(struct mlx5_ib_srmc),GFP_KERNEL);
        memcpy(srmc->dgid.raw,dgid.raw,sizeof(srmc->dgid.raw));

        //将srmc 加入到srmc_head中
        if(sched->srmc_head ==NULL){
            sched->srmc_head = srmc;
        }
        else{
            srmc->next = sched->srmc_head->next;
            sched->srmc_head->next = srmc;
        }
    }
    //srmc->tgt_cb.refcnt == 0
    srmc->tgt_cb.refcnt = 1;
    srmc->tgt_cb.cm_id = cm_id;
    srmc->tgt_cb.state = CONNECT_REQUEST;
    mutex_unlock(&sched->srmc_lock);
    cb = &srmc->tgt_cb;

    cb->txdepth = server_cb->txdepth;
    init_waitqueue_head(&cb->sem);
    cb->server = 1;

    //create pd
    cb->pd = ib_alloc_pd(cb->cm_id->device,0);
    if(IS_ERR(cb->pd)){
        printk(KERN_ERR "alloc pd failed\n");
        ret = PTR_ERR(cb->pd);
        goto err0;
    }
    DEBUG_LOG("alloc pd\n");

    //create cq
    memset(&cq_attr,0,sizeof cq_attr);
    cq_attr.cqe = cb->txdepth;
    cq_attr.comp_vector = 0;
    //change to event?
    cb->cq = ib_create_cq(cb->cm_id->device,NULL,NULL,NULL,&cq_attr);
    if(IS_ERR(cb->cq)){
        printk(KERN_ERR "ib_create_cq failed\n");
        ret = PTR_ERR(cb->cq);
        goto err1;
    }
    DEBUG_LOG("created cq %p\n", cb->cq);

    //create qp
    memset(&init_attr, 0, sizeof(init_attr));
    //init_attr.cap.max_send_wr = cb->txdepth;
    init_attr.cap.max_recv_wr = cb->txdepth;

    /* For flush_qp() */
    // init_attr.cap.max_send_wr++;
    // init_attr.cap.max_recv_wr++;

    init_attr.cap.max_recv_sge = 1;
    //init_attr.cap.max_send_sge = 1;
    //init_attr.send_cq = cb->cq;
    init_attr.recv_cq = cb->cq;
    init_attr.sq_sig_type = IB_SIGNAL_REQ_WR;
    init_attr.qp_type = IB_QPT_XRC_TGT;

    cb->xrcd = ((struct srm_cb*)cm_id->context)->xrcd;
    init_attr.xrcd = cb->xrcd;
    DEBUG_LOG("xrcd = %p,txdepth = %d\n",init_attr.xrcd,init_attr.cap.max_recv_wr);
    if(cb->xrcd==NULL){
        pr_err("xrcd is NULL\n");
        ret = -1;
        goto err2;
    }

    ret = rdma_create_qp(cb->cm_id,cb->pd,&init_attr);
    if(!ret)
        cb->qp = to_mqp(cb->cm_id->qp);
    else{
        pr_err("server rdma_create_qp failed,error:%d\n",ret);
        goto err2;
    }
    DEBUG_LOG("created qp %p\n", cb->qp);


    //accept
    ret = srm_accept(cb);
    if(ret){
        pr_err("accept failed\n");
        goto err3;
    }
    DEBUG_LOG("accept\n");


    cm_id->context = (void*)&srmc->tgt_cb;


    DEBUG_LOG("srm_create_connection success\n");
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
    return ret;

}
static int srm_cma_event_handler(struct rdma_cm_id *cma_id,
									struct rdma_cm_event *event)
{
	int ret;
	struct srm_cb *cb = cma_id->context;

	DEBUG_LOG("cma_event type %d cma_id %p (%s)\n", event->event, cma_id,
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
        kthread_run(srm_create_connection,cma_id, "server connection thread");
		DEBUG_LOG("child cma %p\n", cma_id);
		break;

	case RDMA_CM_EVENT_ESTABLISHED:
		DEBUG_LOG("ESTABLISHED\n");
        cb->state = CONNECTED;
		wake_up_interruptible(&cb->sem);
		break;

	case RDMA_CM_EVENT_ADDR_ERROR:
	case RDMA_CM_EVENT_ROUTE_ERROR:
	case RDMA_CM_EVENT_CONNECT_ERROR:
	case RDMA_CM_EVENT_UNREACHABLE:
	case RDMA_CM_EVENT_REJECTED:
		printk(KERN_ERR "cma event %d, error %d,reject msg:%s\n", event->event,
			   event->status,rdma_reject_msg(cma_id,event->status));
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

static int srm_connect_client(struct srm_cb *cb)
{
	struct rdma_conn_param conn_param;
	int ret;

	memset(&conn_param, 0, sizeof conn_param);
	conn_param.responder_resources = 1;
	conn_param.initiator_depth = 1;
	conn_param.retry_count = 5;

	ret = rdma_connect(cb->cm_id, &conn_param);
	if (ret)
	{
		printk(KERN_ERR  "rdma_connect error %d\n", ret);
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

int create_srmc_qp_cm(struct srm_cb *cb,struct ib_pd *pd,union ib_gid *dgid){
    int ret ;
    struct ib_cq_init_attr cq_attr;
    struct ib_qp_init_attr init_attr;

    cb->addr_str = IP_ADDR;
    cb->port = 12345;
    if(!(ret = in4_pton(cb->addr_str, -1, cb->addr, -1, NULL))){
        printk(KERN_ERR "in4_pton error %d\n",ret);
        ret = -1;
        goto out;
    }
    DEBUG_LOG("Expected IPv4 address: %u.%u.%u.%u\n",
               cb->addr[0], cb->addr[1],cb->addr[2], cb->addr[3]);
    mlx5_ib_gid2ip(cb->addr,dgid);
    DEBUG_LOG("Real IPv4 address: %u.%u.%u.%u\n",cb->addr[0],cb->addr[1],cb->addr[2], cb->addr[3]);
    cb->server = 0 ;
    init_waitqueue_head(&cb->sem);
    cb->txdepth = 256;
    cb->pd = pd;

    cb->cm_id = rdma_create_id(&init_net,srm_cma_event_handler,cb,RDMA_PS_TCP,IB_QPT_XRC_INI);
    if (IS_ERR(cb->cm_id))
	{
		ret = PTR_ERR(cb->cm_id);
		printk(KERN_ERR "rdma_create_id error %d\n", ret);
		goto out;
	}
    DEBUG_LOG("created cm_id %p\n", cb->cm_id);
    ret = srm_bind_client(cb);
    if(ret){
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



    //create cq
    memset(&cq_attr,0,sizeof cq_attr);
    cq_attr.cqe = cb->txdepth;
    cq_attr.comp_vector = 0;
    //change to event?
    cb->cq = ib_create_cq(cb->cm_id->device,NULL,NULL,NULL,&cq_attr);
    if(IS_ERR(cb->cq)){
        printk(KERN_ERR "ib_create_cq failed,cq:%s\n",PTR_ERR(cb->cq));
        ret = PTR_ERR(cb->cq);
        goto err0;
    }

    //create qp
	memset(&init_attr, 0, sizeof(init_attr));
	init_attr.cap.max_send_wr = cb->txdepth;
	init_attr.cap.max_recv_wr = cb->txdepth;

	/* For flush_qp() */
	//init_attr.cap.max_send_wr++;
	//init_attr.cap.max_recv_wr++;

	init_attr.cap.max_recv_sge = 1;
	init_attr.cap.max_send_sge = 1;
	init_attr.qp_type = IB_QPT_XRC_INI;
	init_attr.send_cq = cb->cq;
	init_attr.recv_cq = cb->cq;
	init_attr.sq_sig_type = IB_SIGNAL_REQ_WR;
    ret = rdma_create_qp(cb->cm_id,pd,&init_attr);
    if(!ret)
        cb->qp = to_mqp(cb->cm_id->qp);
    else{
        pr_err("rdma_create_qp failed,error:%d\n",ret);
        goto err1;
    }
    DEBUG_LOG("created qp %p\n", cb->qp);
        
    // //alloc mr and buf
    // ret = mlx5_sched_alloc_mr(cb,pd);
    // if(ret){
    //     pr_err("alloc mr failed,error:%d\n",ret);
    //     goto err2;
    // }

    //modify qp etc
    ret = srm_connect_client(cb);
    if(ret){
        pr_err("connect client failed,error:%d\n",ret);
        goto err3;
    }

    //reg mr and bind buf
    // ret = mlx5_sched_reg_mr(cb->mr,cb->qp,cb->dma_buf,cb->buf_sz,cb->page_list_len);
    // if(ret){
    //     pr_err("reg mr failed,error:%d\n",ret);
    //     goto err4;
    // }
    // DEBUG_LOG("client reg mr success\n");


    
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


    return ret?0:cb->qp->ibqp.qp_num; 
err4:
    rdma_disconnect(cb->cm_id);
err3:
    //TODO:free buf and mr resources.
    //ib_sched_free_buf(cb);
err2:
    ib_destroy_qp(&cb->qp->ibqp);
err1:
    ib_destroy_cq(cb->cq);
err0:
    rdma_destroy_id(cb->cm_id);
out:
    return ret?0:cb->qp->ibqp.qp_num;
}

int srm_bind_server(struct srm_cb *cb){
    struct sockaddr_storage sin;
	int ret;

	fill_sockaddr(&sin, cb);

	ret = rdma_bind_addr(cb->cm_id, (struct sockaddr *)&sin);
	if (ret)
	{
		printk(KERN_ERR  "rdma_bind_addr error %d\n", ret);
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
int mlx5_sched_run_server(struct srm_cb *cb){
    pr_info("srm server is running\n");
    int ret;
    struct ib_cq_init_attr cq_attr;
    struct ib_qp_init_attr init_attr;
    struct ib_pd *pd;

    cb->addr_str = IP_ADDR;
    cb->port = 12345;
    if(!(ret = in4_pton(cb->addr_str, -1, cb->addr, -1, NULL))){
        printk(KERN_ERR "in4_pton error %d\n",ret);
        ret = -1;
        goto out;
    }
    cb->server = 1;
    init_waitqueue_head(&cb->sem);
    cb->txdepth = 256;

    cb->cm_id = rdma_create_id(&init_net, srm_cma_event_handler, cb, RDMA_PS_TCP, IB_QPT_XRC_TGT);
	if (IS_ERR(cb->cm_id))
	{
		ret = PTR_ERR(cb->cm_id);
		printk(KERN_ERR "rdma_create_id error %d\n", ret);
		goto out;
	}
	DEBUG_LOG("created cm_id %p\n", cb->cm_id);

    ret = srm_bind_server(cb);
    if(ret){
        printk(KERN_ERR "bind server failed\n");
        goto err0;
    }
    DEBUG_LOG("bind server\n");

    DEBUG_LOG("rdma_listen\n");
	ret = rdma_listen(cb->cm_id, 10);
	if (ret)
	{
		printk(KERN_ERR "rdma_listen failed: %d\n", ret);
		goto err0;
	}

    wait_event_interruptible(cb->sem,kthread_should_stop());
    pr_info("srm server stop\n"); 
    
err0:
    rdma_destroy_id(cb->cm_id);
out:
    wait_event_interruptible(cb->sem, kthread_should_stop());
    return ret;
}

void ib_sched_free_buf(struct srm_cb *cb){
    DEBUG_LOG("ib_sched_free_buf\n");
    ib_dereg_mr(cb->mr);
    dma_unmap_single(cb->pd->device->dma_device, dma_unmap_addr(cb,dma_mapping), cb->buf_sz, DMA_BIDIRECTIONAL);
    kfree(cb->buf);
}
int sched_hash_gid(union ib_gid *gid,int n){
    u32 hash = jhash(gid->raw,sizeof(gid->raw),0);
    return hash%n;
}