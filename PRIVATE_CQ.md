# 每 KQP 独立发送 CQ（无换路版本）

本分支保留 PRIVATE_CQ，已移除多 QP 换路状态机、备用路径池、冻结位、
路由版本、迁移完成映射以及对应 UAPI。不是仅关闭运行时检测开关。

## 配置

在 `drivers/infiniband/hw/mlx5/scheduler.h` 中设置：

```c
#define MLX5_SRM_ENABLE_PRIVATE_CQ 1
#define MLX5_SRM_PRIVATE_CQ_POLL_BUDGET 256
```

独立 CQ 同时支持 `MLX5_SRM_ENABLE_CQE_SIMPLIFY=0/1`，该简化开关仍须
与用户驱动保持一致。独立 CQ 本身是内核配置，不需要为它增加用户态开关。
共享 CQ 可以通过 PRIVATE_CQ=0 恢复，不必改变简化开关。

本次保留现有用户 DB、stride、大小流、limit_batch、统计配置以及
默认关闭的时延 CQ 优先功能。不把这些独立功能当作换路一起移除。

## 数据路径

- 每个物理 KQP 一个发送 CQ，CQ 索引直接使用 KQP 索引。
- CQ 初始按 SQ 请求深度创建；QP 创建后检查并按实际可预留 SQ 窗口扩容。
- 调度器通过该 SQ 的 `db_tail - cq_complete_idx` 发现待完成请求。
  用户 DB 无需额外 syscall 通知；调度器扫描时能发现它。
- 每次 poll 只处理有限数量的 CQE；不足预算也立即返回。
- 简化模式：poll 内逐 CQE 验证 QPN/opcode/counter，poll 结束时更新 CQ CI、
  SQ 完成游标，再一次性发布 `cons_idx`。
- 非简化模式：复用 `mlx5_ib_poll_cq_with_cqe()`，用 KQP/原始下标和
  publish token 查找用户 CQ，保留逐 CQE 的复制、uidx/owner 发布及错误信息。
  用户 CQE 发布后，才刷新 `cons_idx` 回收 SQ、返还全局信用。
  此模式每次最多处理 `min(PRIVATE_CQ_POLL_BUDGET, SQ_DEPTH)` 个 CQE，
  后者是调度器 WC/CQE 临时数组的容量；不会为了凑满预算等待。
- 两种模式均按 counter 跨度返还完成信用，而非简单按 CQE 个数返还；
  CQ poll 支持一个 CQE 覆盖多个 WR。无备用路径扫描、迁移元数据查询或
  逻辑/物理完成位置转换。
- 不等待单个 CQ 排空：尚有请求的 CQ 回到轮询队列，继续轮询其他 CQ。
- 信用满时仍访问其他 SQ，避免漏掉用户已 DB、尚未加入轮询队列的 CQ。

无换路时物理发送 KQP 数为 `num_kqps * MLX5_SRM_KERNEL_QP_LEVELS`。
关闭大小流为 num_kqps；开启大小流为两倍。不再有两活动、两备用的路径池。

## 回退后的构建与安装

这次回退改变了双方共享头文件与映射响应，两端驱动必须配套重新构建。
现有 build 目录和已加载模块不会因源码修改自动更新。

```bash
cd /home/lingbo11/zxm/rdma-core
bash build.sh

# 停止应用后，在维护窗口执行；包含安装与重载模块。
cd /home/lingbo11/zxm/rdma-kerndriver
sudo bash kernel_make.sh
```

仍使用原有构建脚本，没有新安装前缀。应用原有编译脚本可继续使用。

## 验证

```bash
bash tests/run_private_cq_offline.sh
bash tests/run_latency_cq_offline.sh
python3 tests/check_srm_kernel_compile.py --private-cq 1 \
  --units cq scheduler qp main --wqe-timing
python3 tests/check_srm_kernel_compile.py --private-cq 0 \
  --units cq scheduler qp main
```

离线测试提取生产函数，覆盖空/短批次、预算 1/3/256、64B/128B CQE、
16/32/64 位 counter 回绕、累计完成、错误/重复 CQE、resize 和 DB 快照。
非简化分发测试以模拟 CQ 输入检查生产调度函数：单次预算和数组容量限制、
逐 CQE 路由（包括错误 CQE）、用户 CQ 发布后再返还信用、累计完成跨度、
空 CQ/错误返回以及后续用户 DB 重新激活。编译矩阵包含独立 CQ 的两种模式。
编译检查只在临时目录生成对象，不安装或加载模块，不代表真机吞吐验收。

重装后可用以下命令查看独立 CQ 初始化日志，并留意日志时间戳：

```bash
sudo dmesg | grep -E 'SRM CQ mode=|SRM private CQ ready'
```

## 恢复换路版本

两个仓库均保留分支 `backup/tmp-before-reroute-removal-20260921`，
分别指向回退前的 b521fd69（用户驱动）、000f3f7（内核驱动）。
回退以当前 tmp 工作树中的修改呈现，没有改写其他分支。
