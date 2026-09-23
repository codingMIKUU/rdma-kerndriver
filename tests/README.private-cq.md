# MVAPICH 分支：每个物理 KQP 独立发送 CQ

## 开关

`drivers/infiniband/hw/mlx5/scheduler.h`：

```c
#define MLX5_SRM_ENABLE_PRIVATE_CQ 0           /* 默认关闭；改为 1 启用 */
#define MLX5_SRM_PRIVATE_CQ_POLL_BUDGET 256    /* 每次最多处理的硬件 CQE 数 */
```

只改变 Hollow 内核发送 CQ 的分配与轮询，不改变普通 RC/XRC 的应用 CQ，
不改变 MVAPICH 的 XRC SRQ 接收 CQ、用户 ABI 或用户驱动配置。
现有 `MLX5_SRM_ENABLE_CQE_SIMPLIFY` 继续决定完成的交付方式：

| CQE 模式 | PRIVATE_CQ=0 | PRIVATE_CQ=1 |
| --- | --- | --- |
| 0：软件完成事件 | 原共享 CQ + 逐 CQE 事件分发 | 独立 CQ + 原事件分发 |
| 1：简化水位 | 原共享 CQ + 逐 CQE 发布水位 | 独立 CQ + 每个非空 poll 发布一次水位 |
| 2：直接 64B CQE | 原共享 CQ + 原生 CQE 分发 | 独立 CQ + 原生 CQE 分发 |

本分支由内核 attach 响应协商交付模式，不要套用历史分支要求用户头文件
另设 CQE 简化宏的操作。模式 2 原有的 provider 能力检查和直接 CQE
格式要求仍保留，并非切换 PRIVATE_CQ 就能忽略这些检查。

## 逻辑

- 每个 worker 的 CQ 表用全局 KQP slot 索引，覆盖所有已建立的远端节点、
  大小流两个级别和 lane；不能只用 peer 内的局部下标。
- 全部建链成功时，发送 CQ 数等于物理发送 KQP 数，不改变 KQP 数或选路。
  每个远端节点 `num_kqps * MLX5_SRM_KERNEL_QP_LEVELS` 个发送 CQ。
- CQ 按请求 SQ 深度初建，QP 建好后按实际 `sq.wqe_cnt * 2 / 3` 的可预留
  窗口检查/扩容，保证用户直接 DB 时 CQ 也能容纳全部请求的完成。
  活动窗口必须小于 65536，保证 16 位 WQE counter 可无歧义扩展。
- 调度器通过每条 SQ 的 `db_tail - cq_complete_idx` 发现用户/内核 DB；
  不增加 syscall，不获取用户 DB owner，不使用全 worker 信用代替本 SQ 完成数。
- 一次最多消费 `PRIVATE_CQ_POLL_BUDGET` 个 CQE；不足预算即返回。
  完成信用按 WQE counter 跨度计算，支持多个 unsignaled WR + 一个 signaled WR。
- 模式 1 直接使用已知 KQP，在局部变量累计硬件游标，更新 CQ CI 后一次
  发布 `cons_idx`；仍检查每个硬件 CQE 的 owner/opcode/QPN/counter 和首个错误。
- 模式 0/2 保留现有用户完成队列背压：队列满时不消费该 CQE、不回收
  对应 SQ slot、不返还信用，但让其他独立 CQ 继续轮询。
- 独立 CQ 每轮只处理有限批次，未完成的放回轮询环。全局信用耗尽也不
  阻塞后续 SQ 扫描，否则可能漏掉尚未入环的用户直接 DB 请求。
- 新增的 CQ 代表表在堆上分配，不把 NUM_SRMC 个指针放到内核线程栈；
  模块停止仍先停止 worker、销毁 KQP，再逐个销毁 CQ。初始化失败也沿用
  worker CQ 表的集中清理，不重复销毁。

关闭时新增轮询分支由预处理器移除，保留原共享 CQ 路径。
内存占用与吞吐收益需实测：独立 CQ 增加硬件 CQ 数，并不保证所有负载更快。

## 离线验证（不会安装模块）

```bash
python3 tests/test_srm_private_cq.py
python3 tests/test_srm_cq_dispatch.py
python3 tests/test_srm_multipeer.py
python3 tests/test_srm_large_direct_db.py
python3 tests/check_srm_kernel_compile.py
```

编译矩阵：PRIVATE_CQ=0/1 × CQE 模式 0/1/2 × 大小流关闭/开启。
编译检查复用现有 Kbuild `.o.cmd`，仅在临时目录生成对象文件。
离线测试不替代多机 MPI 正确性、资源释放与吞吐测试。

## 应用配置

停止实验、确认维护窗口后，在本机驱动目录使用原有安装命令：

```bash
sudo bash kernel_make.sh
sudo dmesg | grep -E 'SRM CQ mode=|SRM private CQ ready'
```

本改动不需要重编译 rdma-core 或 MVAPICH；双向 MPI 对照时，各机器分别
设置开关并重建/加载对应模块。检查最新时间戳，避免把旧日志当成当前配置。
模式 1 + 独立 CQ 启动日志包含 `mode=private-per-kqp cq_delivery=1`
和 `publish=poll-exit`。关闭时显示 `mode=shared`。
