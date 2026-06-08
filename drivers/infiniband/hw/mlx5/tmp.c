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
    //     snprintf(pt, 200, "/root/zxm/rdma-kerndriver/fcscale_%ddata_%d.txt", num_kqps, id);

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
    //                      O_WRONLY | O_CREAT | O_TRUNC,
    //                      0644);
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

    const int wqes_limit_sz = 62 * 1024; // 124KB

    int wqe_cur_idx = 0;
    uint64_t wqe_tot_sz = 4096 * WQES_ARR_SZ;
    uint64_t cur_wqes[WQES_ARR_SZ] = {0};
    for (i = 0; i < WQES_ARR_SZ; i++)
    {
        cur_wqes[i] = 4096;
    }
    int64_t target_sz, send_ok;
    int polling_order[][4] =
        {
            {0, 1, 2, 3},
            {1, 0, 2, 3},
            {2, 3, 1, 0},
            {3, 2, 1, 0}};
    int order_idx, l, m, n;
    uint64_t polling_seed;
    polling_seed = 0xdeadbeef;
    uint32_t user_table_val, kernel_table_val, user_level_val;
    int num_user_threads = 0, num_thread_qps, per_thread_qp_nums;

    int ten_level, hund_level;
    int skip_level_arr[4] = {-1, -1, -1, -1}; // 0~4KB,4~10KB,10~100KB,>100KB
    int skip_level_cnt[4] = {0, 0, 0, 0};
    int level;

    struct mlx5_ib_srmc *cq_srmc_tb[CQ_NUM] = {0}; // 保存每个cq对应srmc代表

    int *free_cqe_idx, free_cqe_cnt;
    free_cqe_idx = kmalloc_array(SQ_DEPTH, sizeof(int), GFP_KERNEL);
    free_cqe_cnt = SQ_DEPTH;
    for (i = 0; i < SQ_DEPTH; i++)
    {
        free_cqe_idx[i] = i;
    }

    uint32_t level_owqe_cnt_arr[4] = {0};
    uint32_t level_wqe_cnt, wqe_cnt, user_threads_idx;
    int sending_case; // 对应新的wqe个数和旧的wqe个数的几种情况,0~2代表三种情况，3代表应该break了

    while (!kthread_should_stop())
    {
        if (num_table_qp != sched_group.sqb_cnt || !num_table_qp)
        {
            msleep(0);
            continue;
        }

        num_user_threads = num_table_qp / (4 * sched_group.num_sched);
        num_thread_qps = 4 * sched_group.num_sched;
        per_thread_qp_nums = sched_group.sqb_cnt / sched_group.num_sched;
        // pr_info("num_user_threads:%d\n", num_user_threads);
        break;
    }

    int *level_qp_st_arr;
    level_qp_st_arr = kmalloc_array(4, sizeof(int), GFP_KERNEL);
    memset(level_qp_st_arr, 0, 4 * sizeof(int));

    u8 use_user_idx;

    uint64_t skip_cnt10 = 0, skip_cnt100 = 0, empty_rolling10 = 0, empty_rolling100 = 0;
    uint64_t wqe_sending_target_cnt = 10240;

    while (!kthread_should_stop())
    {
        // for (sqb = sched_group.sq_head, qp_cnt = 0; sqb; sqb = sqb->next, qp_cnt++){
        //     end_time0 = rdtsc();
        //     elapsed_time0 = (end_time0 - start_time0)*1000000000 / cpu_frequency_hz;
        //     start_time0 = rdtsc();
        //     printk(KERN_INFO "用户态sq切换开销elapsed_time0 = %llu ns\n", elapsed_time0);
        // }

        target_sz = wqes_limit_sz - (wqe_tot_sz - cur_wqes[wqe_cur_idx]);
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
        send_ok = 0;
        tfree = 1;
        for (l = 0; l < 4; l++)
        {
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
                    if (pre_srmcs[polling_head] != NULL)
                    {
                        pr_info("cq polling queue exceed queue length\n");
                        // 此时polling队列满，必须poll完
                        while (pre_srmc->sig_cnt)
                        {
                            srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, &free_cqe_cnt);
                        }
                        in_queue[pre_srmc->srmc_idx % (CQ_NUM)] = 0;
                    }
                    else if (pre_srmc->sig_cnt >= SQ_DEPTH || (int)(pre_srmc->ini_cb.qp->sq.head - pre_srmc->ini_cb.qp->sq.tail) >= pre_srmc->ini_cb.qp->sq.max_post)
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

            level = polling_order[order_idx][l];
            // if (level >= 1 && skip_level_arr[level] >= 0)
            // {
            //     // 当前等级检查跳过等级
            //     if (skip_level_cnt[level] < 0)
            //     {
            //         // 代表刚刚降级，还需要轮询
            //         ;
            //     }
            //     else if (skip_level_cnt[level] < (1 << skip_level_arr[level]))
            //     {
            //         skip_level_cnt[level]++;
            //         continue;
            //     }
            //     // pr_info("level:%d,skip_level:%d,skip cnt:%d,skip.\n",level,skip_level_arr[level],skip_level_cnt[level]);
            // }

            user_level_val = smp_load_acquire(&user_level_table[level + 4 * id]);
            level_wqe_cnt = user_level_val - kernel_level_table[level + 4 * id];
            if (!level_wqe_cnt)
            {
                //level_owqe_cnt_arr[level] = level_wqe_cnt;

                // //文件
                // if (level <= 1)
                // {
                //     skip_cnt10++;
                // }
                // else
                // {
                //     skip_cnt100++;
                // }

                // // 大消息上升退避等级
                // if (level >= 1)
                // {
                //     skip_level_arr[level] = min(2, skip_level_arr[level] + 1);
                //     skip_level_cnt[level] = 0;
                // }

                //                 // 文件
                //                 /* 3. 写数据 */
                //                 len = scnprintf(buf, 256, "level %d skip\n", level);
                // #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
                //                 /* kernel_write 从 5.11+ 内核可用，无需 set_fs */
                //                 ret = kernel_write(filp, buf, len, &pos);
                // #else
                //                 ret = vfs_write(filp, buf, len, &pos);
                // #endif
                //                 if (ret < 0)
                //                     pr_err("write_int_to_file: write error %d\n", ret);
                continue;
            }

            // if(level_owqe_cnt_arr[level] != level_wqe_cnt){
            //     if(level_owqe_cnt_arr[level] == 0){
            //         //在上一次遍历时该等级没有wqe，则直接用下标表
            //         user_threads_idx = smp_load_acquire(&user_idx_table[level + 4 * id]);
            //         sending_case = 0;
            //     }
            //     else{
            //         //上次遍历有wqe，则先用顺序遍历的下标，空转再用下标表，防止饥饿问题
            //         user_threads_idx = srm_fastrand(&polling_seed) % num_user_threads;
            //         sending_case = 1;
            //     }
            // }
            // else {
            //     //wqe个数相较于上一次没有变化，直接使用上次下标
            //     user_threads_idx = srm_fastrand(&polling_seed) % num_user_threads;
            //     sending_case = 2;
            // }

            // // 插入获取屏障：确保读取b后，c的最新值已可见
            // smp_rmb();  // 读内存屏障，阻止读重排

            k = level * sched_group.num_sched + id + srm_fastrand(&polling_seed)%num_user_threads * 4 * sched_group.num_sched;
            for (m = 0; m < num_user_threads; m++, k = (k + num_thread_qps) % sched_group.sqb_cnt)
            {

                // n = polling_order[order_idx][l] * num_user_threads + k / 6;
                n = k / (4 * sched_group.num_sched) + level * num_user_threads + id * per_thread_qp_nums;
                user_table_val = smp_load_acquire(&user_wqe_table[n]);
                kernel_table_val = kernel_wqe_table[n];
                if (user_table_val == kernel_table_val)
                {
                    // 此时该qp中没有wqe

                    // //文件
                    // if (level <= 1)
                    //     empty_rolling10++;
                    // else
                    //     empty_rolling100++;

                    //                     // 文件
                    //                     /* 3. 写数据 */
                    //                     len = scnprintf(buf, 256, "skip, k:%d,target_size:%lld,idx for table:%d,user_table_val:%u,"
                    //                                               "kernel_table_val:%u\n",
                    //                                     k, target_sz, n, user_table_val, kernel_table_val);
                    // #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
                    //                     /* kernel_write 从 5.11+ 内核可用，无需 set_fs */
                    //                     ret = kernel_write(filp, buf, len, &pos);
                    // #else
                    //                     ret = vfs_write(filp, buf, len, &pos);
                    // #endif
                    //                     if (ret < 0)
                    //                         pr_err("write_int_to_file: write error %d\n", ret);

                    // if (use_user_idx)
                    // {
                    //     use_user_idx = 0;
                    //     // pr_err("k:%d,should not use user idx and not found wqe\n", k);

                    //     // // 文件
                    //     // len = scnprintf(buf, 256, "k:%d,should not use user idx and not found wqe\n", k);
                    //     // ret = vfs_write(filp, buf, len, &pos);
                    //     // if (ret < 0)
                    //     //     pr_err("write_int_to_file: write error %d\n", ret);
                    // }

                    // //文件
                    // if (level <= 1)
                    // {
                    //     skip_cnt10++;
                    // }
                    // else
                    // {
                    //     skip_cnt100++;
                    // }

                    // if(sending_case == 0){
                    //     sending_case = 2;
                    //     //pr_err("should not use user idx and not found wqe\n");
                    //     user_threads_idx = srm_fastrand(&polling_seed) % num_user_threads;
                    //     k = level * sched_group.num_sched + id + user_threads_idx * 4 * sched_group.num_sched;
                    // }
                    // else if (sending_case == 1){
                    //     sending_case = 0;//接下来访问下标表
                    //     user_threads_idx = smp_load_acquire(&user_idx_table[level + 4 * id]);
                    //     k = level * sched_group.num_sched + id + user_threads_idx * 4 * sched_group.num_sched;
                    // }else if(sending_case == 2){
                    //     k = (k + num_thread_qps) % sched_group.sqb_cnt;
                    // }else if(sending_case == 3){
                    //     //不应该在这里
                    //     pr_err("should not be here,sending_case is 3,should break after sending\n");
                    //     break;
                    // }
                    continue;
                }

                // if(level == 1 && sending_case == 2){
                //     //这种情况需要排空wqe
                //     wqe_cnt = user_table_val - kernel_table_val;
                //     sending_case = 3;
                // }

                sqb = sched_group.sqb_arr[k];
                if (sqb == NULL)
                {
                    pr_err("sqb %d is NULL\n", k);
                    continue;
                }

                uidx = sqb->cur_post & (sqb->wqe_cnt - 1);
                uctrl = useg = (sqb->buf + (uidx << 6)); // 64B的wqe
                imm = smp_load_acquire(&uctrl->imm);     // 内存屏障，为1表示有wr
                if (!imm)
                {
                    // DEBUG_LOG("imm is 0\n");
                    // tfree = 1;
                    // if(id == 0){
                    //     cnt++;
                    //     if(cnt>10){
                    //         pr_info("stuck in 1\n");
                    //         cnt = 0;
                    //     }
                    // }

                    //                 // 文件
                    //                 /* 3. 写数据 */
                    //                 len = scnprintf(buf, 128, "imm = 0, k:%d,target_size:%lld,idx for table:%d,user_table_val:%u,"
                    //                     "kernel_table_val_:%u\n", k, target_sz,n,user_table_val,kernel_table_val);
                    // #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
                    //                 /* kernel_write 从 5.11+ 内核可用，无需 set_fs */
                    //                 ret = kernel_write(filp, buf, len, &pos);
                    // #else
                    //                 ret = vfs_write(filp, buf, len, &pos);
                    // #endif
                    //                 if (ret < 0)
                    //                     pr_err("write_int_to_file: write error %d\n", ret);

                    pr_err("imm should not be zero\n");
                    continue;
                }

                //                 end_time0 = rdtsc();
                //                 elapsed_time0 = (end_time0 - start_time0)*1000000000 / cpu_frequency_hz;
                //                 start_time0 = rdtsc();

                // if(sched_hash_ip((char*)&imm, sched_group.num_sched) != id){
                //     //DEBUG_LOG("id is not equal, id is %d\n",id);
                //     // if(id == 0){
                //     //     cnt2++;
                //     //     if(cnt2>10){
                //     //         pr_info("stuck in 2\n");
                //     //         cnt2 = 0;
                //     //     }
                //     // }
                //     break;
                // }
                // 192.168.1.x
                //             if (id != ((imm >> 24) & 0xFF)) // 判断WR是否属于当前调度器
                //             {
                //                 //                      len = scnprintf(buf, 64, "%d %d %d %llu\n", 2, 2, sqb->qpn, 2);
                //                 // #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
                //                 //                     /* kernel_write 从 5.11+ 内核可用，无需 set_fs */
                //                 //                     ret = kernel_write(filp, buf, len, &pos);
                //                 // #else
                //                 //                     ret = vfs_write(filp, buf, len, &pos);
                //                 // #endif
                //                 //                     if (ret < 0)
                //                 //                         pr_err("write_int_to_file: write error %d\n", ret);

                // //                 // 文件
                // //                 /* 3. 写数据 */
                // //                 len = scnprintf(buf, 128, "not this thread,k:%d,"
                // //                     "total srm qp:%d,target_sz:%lld\tuser_wqe_table for n %d:%d,kern_wqe_table:%d\n", k,
                // //                     sched_group.sqb_cnt, target_sz,n, user_table_val, kernel_table_val);
                // // #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
                // //                 /* kernel_write 从 5.11+ 内核可用，无需 set_fs */
                // //                 ret = kernel_write(filp, buf, len, &pos);
                // // #else
                // //                 ret = vfs_write(filp, buf, len, &pos);
                // // #endif
                // //                 if (ret < 0)
                // //                     pr_err("write_int_to_file: write error %d\n", ret);

                //                 continue;
                //             }

                // 我觉得还是要开这个，调度更公平
                // if (sched_size > SCHED_SIZE_LIMIT)
                // {
                //     DEBUG_LOG("sched once\n");
                //     break;
                // }

                sqb->cur_post++; // 放在kern_table++、imm置0之前，让用户更快取新的。需要保证一下次序在后俩个之前？
                smp_store_release(&kernel_wqe_table[n], kernel_table_val + 1);
                smp_store_release(&uctrl->imm, 0);

                DEBUG_LOG("uidx:%d\n", uidx);

                // imm = (imm & 0x00FFFFFF) | (1 << 24); //将1放在imm的高8位
                ((char *)(&imm))[3] = 1;
                memcpy(gid.raw + 12, &imm, 4);
                // gid.raw[15] = 1;

                DEBUG_LOG("found wr's gid.interface_id:%llx,subnet_prefix:%llx\n", gid.global.interface_id, gid.global.subnet_prefix);

                useg += sizeof(struct mlx5_wqe_ctrl_seg);
                uxrc = (struct mlx5_wqe_xrc_seg *)useg;
                useg += sizeof(struct mlx5_wqe_xrc_seg);
                uraddr = (struct mlx5_wqe_raddr_seg *)useg;
                useg += sizeof(struct mlx5_wqe_raddr_seg);
                udata = (struct mlx5_wqe_data_seg *)useg;

                length = ntohl(udata->byte_count);
                DEBUG_LOG("length:%d\n", length);

                // pr_info("sending wqes,k:%d,length:%d,total srm qp:%d,target_sz:%d\n", k, length, sched_group.sqb_cnt, target_sz);
                // pr_info("user_wqe_table for n %d:%d,kern_wqe_table:%d\n", n, user_wqe_table[n], kernel_wqe_table[n]);

                // end_cycles = rdtsc();
                // elapsed_cycles = end_cycles - start_cycles;
                // elapsed_ns = (elapsed_cycles * 1000000000) / cpu_frequency_hz;
                // start_cycles = rdtsc();

                //                 // 文件
                //                 /* 3. 写数据 */
                //                 len = scnprintf(buf, 256, "sending wqes,k:%d,"
                //                                           "length:%d,total srm qp:%d,target_sz:%lld\tuser_wqe_table for n %d:%d,kern_wqe_table:%d,kern_wqe_val:%d,elapsed_ns:%llu\n",
                //                                 k, length,
                //                                 sched_group.sqb_cnt, target_sz, n, user_table_val, kernel_wqe_table[n], kernel_table_val, elapsed_ns);
                // #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
                //                 /* kernel_write 从 5.11+ 内核可用，无需 set_fs */
                //                 ret = kernel_write(filp, buf, len, &pos);
                // #else
                //                 ret = vfs_write(filp, buf, len, &pos);
                // #endif
                //                 if (ret < 0)
                //                     pr_err("write_int_to_file: write error %d\n", ret);

                hash_id = sched_hash_ip((char *)&imm, NUM_SRMC); // 查找目标SRMC
                found = 0;

                // // 时延线程在第17个
                // if (k != 16){
                //     //rd = prandom_u32_max(num_kqps - 1);
                //     rd = srm_fastrand(&srm_seed)%(num_kqps-1);
                // }
                // else{
                //     rd = num_kqps - 1;
                //     //pr_info("lat thread length:%d\n",length);
                // }
                rd = prandom_u32_max(num_kqps);

                for (i = 0; i < NUM_SRMC; i++)
                {
                    j = (i + hash_id) % NUM_SRMC;
                    srmc = sched->srmc_tb[j];
                    if (srmc == NULL)
                    {
                        pr_err("Unexpected:No srmc found for this wr\n");
                        goto err;
                    }
                    if (memcmp(srmc->dgid.raw, gid.raw, sizeof(srmc->dgid.raw)) == 0)
                    {
                        DEBUG_LOG("found srmc,gid.interface_id:%llx,subnet_prefix:%llx\n", srmc->dgid.global.interface_id, srmc->dgid.global.subnet_prefix);
                        if (!srmc->ini_cb.qp)
                        {
                            pr_err("Unexpected:ini qp for this srmc is NULL\n");
                            goto err;
                        }
                        found = 1;
                        j = (j + rd) % NUM_SRMC; // 随机选择一个srmc
                        srmc = sched->srmc_tb[j];
                        break;
                    }
                }
                if (!found)
                {
                    pr_err("Unexpected:No srmc found for this wr\n");
                    goto err;
                }

                wqe_tot_sz -= cur_wqes[wqe_cur_idx];
                wqe_tot_sz += length;
                cur_wqes[wqe_cur_idx] = length;
                wqe_cur_idx = (wqe_cur_idx + 1) % WQES_ARR_SZ;

                srmc->cul_pending_bytes += length;
                // sched_size += length;
                srmc->pending_bytes += length;
                tfree = 0;

                qp = srmc->ini_cb.qp;
                spin_lock_irqsave(&qp->sq.lock, flags);
                next_fence = 0;
                fence = qp->next_fence;

                if (unlikely(mlx5r_wq_overflow(&qp->sq, 1, qp->ibqp.send_cq)))
                {
                    pr_err("sq overflow\n");
                    spin_unlock_irqrestore(&qp->sq.lock, flags);
                    goto err;
                }

                idx = qp->sq.cur_post & (qp->sq.wqe_cnt - 1);
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
                    if (free_cqe_cnt <= 0)
                    {
                        pr_err("free_cqe_cnt <= 0, cq exceed\n");
                        goto err;
                    }
                    uidx = free_cqe_idx[--free_cqe_cnt];
                    qp->sq.wrid[idx] = uidx;
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

                // // 文件
                // wqe_sending_target_cnt--;
                // if (wqe_sending_target_cnt == 0)
                // {
                //     wqe_sending_target_cnt = 10240;
                //     len = scnprintf(buf, 256, "sending %llu wqes,skip time for 10KB:%llu,100KB:%llu,empty rolling for 10KB:%llu, for 100KB:%llu.\n",
                //                     wqe_sending_target_cnt, skip_cnt10, skip_cnt100, empty_rolling10, empty_rolling100);
                //     ret = vfs_write(filp, buf, len, &pos);
                //     if (ret < 0)
                //         pr_err("write_int_to_file: write error %d\n", ret);
                //     skip_cnt10 = 0;
                //     skip_cnt100 = 0;
                //     empty_rolling10 = 0;
                //     empty_rolling100 = 0;
                // }

                if (sig)
                {
                    if (!in_queue[srmc->srmc_idx % (CQ_NUM)])
                    {
                        if (pre_srmcs[polling_head] != NULL)
                        {
                            pre_srmc = pre_srmcs[polling_tail];
                            pre_srmcs[polling_tail] = NULL;
                            polling_tail = (polling_tail + 1) % SRMC_POLLING_CNT;
                            pr_info("err:exceed queue length\n");
                            // 此时队列满，必须poll到,此时head = (tail-1+polling_cnt)%polling_cnt
                            while ((ret = srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, &free_cqe_cnt)) != -1)
                            {
                                ;
                            }
                            in_queue[pre_srmc->srmc_idx % (CQ_NUM)] = 0;
                        }

                        if (cq_srmc_tb[srmc->srmc_idx % (CQ_NUM)] == NULL)
                            cq_srmc_tb[srmc->srmc_idx % (CQ_NUM)] = srmc;

                        pre_srmcs[polling_head] = cq_srmc_tb[srmc->srmc_idx % (CQ_NUM)];
                        polling_head = (polling_head + 1) % SRMC_POLLING_CNT;
                        in_queue[srmc->srmc_idx % (CQ_NUM)] = 1;
                    }

                    pre_srmc = cq_srmc_tb[srmc->srmc_idx % (CQ_NUM)];
                    if (pre_srmc == NULL)
                    {
                        pr_err("pre_srmc is null\n");
                    }

                    DEBUG_LOG("send signaled\n");
                    pre_srmc->wqe_infos[uidx].qpn = sqb->qpn;
                    pre_srmc->wqe_infos[uidx].wqe_counter = sqb->cur_post - 1; // 当前cur_post提前++了，所以减1
                    pre_srmc->wqe_infos[uidx].pending_bytes = pre_srmc->cul_pending_bytes;
                    pre_srmc->wqe_infos[uidx].sqb = sqb;
                    pre_srmc->wqe_infos[uidx].to_user = to_user;
                    pre_srmc->wqe_infos[uidx].byte_cnt = length;
                    pre_srmc->wqe_infos[uidx].valid = 1;
                    pre_srmc->sig_cnt++;
                    // pre_srmc->cur_cqe++;
                    pre_srmc->cul_pending_bytes = 0;

                    // pr_info("sig_cnt++,now sig_cnt:%d\n", pre_srmc->sig_cnt);
                }

                DEBUG_LOG("send finished\n");

                // atomic_inc(&kernel_wqe_table[n]); // 同样提前，在cur_post++之前，以防线程认为还有wqe但找不到的情况
                // sqb->cur_post++;       // 可以提前，更加快
                send_ok = 1;

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
                        if (pre_srmcs[polling_head] != NULL)
                        {
                            pr_info("cq polling queue exceed queue length\n");
                            // 此时polling队列满，必须poll完
                            while (pre_srmc->sig_cnt)
                            {
                                srm_poll_srmc_once(pre_srmc, wc, cqe, free_cqe_idx, &free_cqe_cnt);
                            }
                            in_queue[pre_srmc->srmc_idx % (CQ_NUM)] = 0;
                        }
                        else if (pre_srmc->sig_cnt >= SQ_DEPTH || (int)(srmc->ini_cb.qp->sq.head - srmc->ini_cb.qp->sq.tail) >= srmc->ini_cb.qp->sq.max_post)
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

                // if (level >= 1)
                // {
                //     // 由于轮询到了，降低退避等级,并且再次轮询看是否降级
                //     skip_level_arr[level]--;
                //     skip_level_arr[level] = max(skip_level_arr[level], -1);
                //     skip_level_cnt[level] = -1;
                // }

                //                 //文件
                //                 /* 3. 写数据 */
                //                 int level_tot_wqe_num = calc_level_tot_wqe_num(n,num_user_threads);
                //                 len = scnprintf(buf, 256, "level %d down ,skip level:%d,level's total wqe num:%d\n",level,skip_level_arr[level],level_tot_wqe_num);
                // #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
                //                 /* kernel_write 从 5.11+ 内核可用，无需 set_fs */
                //                 ret = kernel_write(filp, buf, len, &pos);
                // #else
                //                 ret = vfs_write(filp, buf, len, &pos);
                // #endif
                //                 if (ret < 0)
                //                     pr_err("write_int_to_file: write error %d\n", ret);

                // // 查看当前wqe是不是最后一个,进行等级表的设置
                // if (user_wqe_table[n] == kernel_wqe_table[n])
                // {
                //     m++;
                //     ret = 0;
                //     for (; m < num_user_threads; m++, k = (k + num_thread_qps) % sched_group.sqb_cnt)
                //     {
                //         // 检查剩下的qp是否为空

                //         n = k / (4 * sched_group.num_sched) + level * num_user_threads + id * per_thread_qp_nums;
                //         if (user_wqe_table[n] != kernel_wqe_table[n])
                //         {
                //             // 不为空
                //             ret = 1;
                //             break;
                //         }
                //     }

                //     // level_table的下标从1开始
                //     user_level_table[level + 4 * id] = ret;
                // }

                kernel_level_table[level + 4 * id]++;
                break;

                // if(sending_case == 0){
                //     sending_case = 4;
                // }
                // else if (sending_case == 1){
                //     //level_qp_st_arr[level] = (level_qp_st_arr[level]+1)%num_user_threads;//直接下一个吗？还是允许连续发有限个？
                //     sending_case = 4;
                // }else if(sending_case == 2){
                //     //level_qp_st_arr[level] = (level_qp_st_arr[level]+1)%num_user_threads;//直接下一个吗？还是允许连续发有限个？
                //     sending_case = 4;
                // }
                // else if(sending_case == 3){
                //     //顺序遍历到level 1的情况，对于level 1的队列，需要排空，继续发送
                //     wqe_cnt--;
                //     if(wqe_cnt == 0){
                //         sending_case = 4;
                //     }
                // }

                // if(sending_case == 4){
                //     //当前情况发送完毕，更新old值,需要使用最新的user_level_table更新
                //     level_owqe_cnt_arr[level] = smp_load_acquire(&user_level_table[level + 4*id]) - kernel_level_table[level + 4*id];
                //     break;
                // }
            }
            if (send_ok)
                break;

            // pr_err("shouldn't find no wqe to send\n");
            //  // 该等级空转，上升skip level，最多退避256次
            //  if (level <= 1)
            //  {
            //      skip_level_arr[level] = min(0, skip_level_arr[level] + 1);
            //  }
            //  else
            //  {
            //      skip_level_arr[level] = min(10, skip_level_arr[level] + 1);
            //  }
            //  skip_level_cnt[level] = 0;

            //             //文件
            //             /* 3. 写数据 */
            //             len = scnprintf(buf, 256, "level %d up ,skip level:%d\n",level,skip_level_arr[level]);
            // #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
            //             /* kernel_write 从 5.11+ 内核可用，无需 set_fs */
            //             ret = kernel_write(filp, buf, len, &pos);
            // #else
            //             ret = vfs_write(filp, buf, len, &pos);
            // #endif
            //             if (ret < 0)
            //                 pr_err("write_int_to_file: write error %d\n", ret);
        }
        cnt += tfree;
        if (cnt % 1000000 == 0)
        {
            cnt++;
            msleep(0);
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
    sched->task = NULL;

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
    sched->task = NULL;

    // // 文件
    // filp_close(filp, NULL);
    // kfree(buf);
    return -1;
}



