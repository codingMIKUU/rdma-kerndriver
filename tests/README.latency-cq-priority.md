# General 时延流 CQ 优先轮询

开关位于 `drivers/infiniband/hw/mlx5/scheduler.h`：

```c
#define MLX5_SRM_ENABLE_LATENCY_CQ_PRIORITY 0
```

改成 `1` 启用；仅内核需要设置这个开关。要求独立 CQ 已开启
（`MLX5_SRM_ENABLE_PRIVATE_CQ=1`，且 CQE 简化开启）。关闭时新增的
优先选择、引用表、bitmap、workspace 字段全部编译掉，保留原轮询路径。

## 行为

- General 的 `run_server_srm()` 仅在 `test_lat_thread=1` 时，把最后一个
  发送线程（`srv_gid == kAppNumServers`）标记为时延流。
- 建逻辑 QP 时用 `IBV_QP_CREATE_SRM_LATENCY_CQ` 传递提示。rdma-core
  将它转换成 mlx5 私有命令标志，不改 verbs 结构体布局，也不在每次发送时增加操作。
- 内核在实际 attach 后，将对应物理 KQP 登记到所属 worker 的优先集合。
  不假定“最后一个线程”对应“最大 QPN”，不改变 KQP 映射。
- 每次 `poll_srmc_inline()` 前先从优先集合轮转挑一个 CQ，最多轮询一批，
  然后仍处理正常 CQ 队列中的一个 CQ。两者相同则只 poll 一次。
  不等待 CQE、不 drain 到空、不占用其他 worker 的 CQ。
- 优先 CQ 不必已在正常队列中：直接通过原有 `db_tail` / 完成游标判定
  outstanding，因而也能发现用户 DB；不增加 syscall。
- 大小流双 QP：两条路径均登记，优先集合内轮转。
- 换路：登记该逻辑组的四条路径，涵盖新活动路径和旧路径排空。
  空闲备用路径没有 outstanding，不访问硬件 CQ。正常轮询仍然保留。
- 销毁逻辑 QP 时撤销引用；多个逻辑 QP 指向同一个 CQ 时，最后一个
  时延标记引用退出才取消优先级。调度器停机仍沿用原 stop-before-free 顺序。

这是 **CQ 服务优先级**，不是时延流独占 QP；同物理 KQP 上其他流的 CQE
也会一同被处理，且不会绕过 SQ 内的队头阻塞。开启会增加轮询工作量，
不能保证吞吐不下降或时延一定改善。

## 构建与运行

需要发送端同时更新三个仓库代码。新的应用标记要求新的 rdma-core 和内核
识别；不要搭配旧内核。接收端不需要为这个发送 CQ trick 单独升级。

当前 General 链接 `rdma-core/build/lib`，沿用此目录：

```bash
cd /home/lingbo11/zxm/rdma-core
cmake --build build --target mlx5 -j8

cd /home/lingbo11/zxm/RDMA-General/sender-scalability
sudo bash do.sh

# 先在 scheduler.h 打开上述宏；停止测试后再安装、重载内核驱动。
cd /home/lingbo11/zxm/rdma-kerndriver
sudo bash kernel_make.sh
```

`kernel_make.sh` 仍是原脚本，会编译、安装、卸载并加载模块；不要在测试中执行。
本次实现没有修改该脚本、环境变量、实验参数、QP 数量、limit_batch 或 DB 策略。

运行必须进入 Hollow 的 `run_server_srm()`：`--use_srm 1 --test_lat_thread 1`。
`--use_srm 0` 是普通 RC，用户直接 poll 自己的 CQ，不经过此内核路径。
注意当前 `run-servers.sh` 的 flags 里写死了 `--use_srm 0`；仅设置脚本外面的
`USE_SRM=1` 不会覆盖它，需要自行改 flags 的该项。实验脚本本次未改。

```bash
sudo dmesg | grep 'SRM latency CQ priority'
```

模块加载应有 `enabled=1`，创建时延流应有
`attach usr_rc=... sched=... worker=... kqp=... qpn=...`。
应用 stderr 同时有 `HRD: latency CQ hint thread=... qp=0`。
若没有 attach 日志，检查是否进入 Hollow 路径、是否重编 General，以及
实际加载的用户库/内核是否已更新。正常 poll 不逐次打印。

## 非硬件验证命令

```bash
cd /home/lingbo11/zxm/rdma-kerndriver
bash tests/run_latency_cq_offline.sh
bash tests/run_private_cq_offline.sh
python3 tests/check_srm_kernel_compile.py --private-cq 1 \
  --latency-cq-priority 1 --units scheduler qp
python3 tests/check_srm_kernel_compile.py --private-cq 1 \
  --latency-cq-priority 0 --units scheduler qp
python3 tests/check_srm_kernel_compile.py --private-cq 1 \
  --latency-cq-priority 1 --reroute 1 --units scheduler qp
python3 tests/check_srm_kernel_compile.py --private-cq 1 \
  --latency-cq-priority 1 --units scheduler --diagnostics --wqe-timing

cd /home/lingbo11/zxm/RDMA-General
g++ -std=c++11 -DNDEBUG -w -march=native -I. \
  -I../rdma-core/build/include -fsyntax-only \
  sender-scalability/main.cc libhrd_cpp/hrd_conn.cc
```

内核编译检查复用现有 Kbuild flags，仅在临时目录产生对象文件；不安装模块。
离线测试直接提取生产轮询函数，覆盖优先顺序、普通 CQ 公平性、重复 CQ 去重、
用户 DB 后正常队列为空、多候选轮转、撤销标记、worker 隔离和开关关闭。
这些验证不代替双机硬件功能及性能测试。
