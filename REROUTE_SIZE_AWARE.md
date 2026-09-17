# 按冻结区间 payload 总量换路（实验分支）

基线：`a76757532c1c454aeb39b031c2de2fa7953240b2`；分支：`srm-reroute-size-aware`。
匹配用户驱动从 `0f4cfea4` 派生，同名分支。没有整体移植 `srm-fast-reroute`。

## 边界与开关

- 双方 `MLX5_SRM_ENABLE_REROUTE` 默认 **0**，定义在各自的
  `rdma/mlx5-srm-reroute.h`。关闭时不进入新增选路、计数、迁移、完成映射路径。
- 首版单 scheduler、单 worker、关闭大小流双 QP 和 ready fastpath。
  沿用这个基线的单远端发送池；第二个不同远端 GID 明确返回 `EOPNOTSUPP`，
  不允许覆盖已有组。多远端、多 worker 未适配。
- `num_kqps` 是逻辑组数。每组 4 个真实 KQP，初始 0/1 活动、2/3 备用。
  默认 32 组意味着 64 活动、64 备用、128 物理 KQP，不能与原来 32 个
  活动 KQP 的结果直接当成算法收益比较。
- READ/WRITE、恰好一个非零长 SGE、非 inline、单 64B WQEBB、显式
  `IBV_SEND_SIGNALED`（可带 FENCE）。其他 WR 在预留前拒绝。
  不支持 SEND、原子、多个 SGE、unsignaled、WQE signature。
- 物理 SQ 不超过 65536 个 BB；用户最多占用深度的 2/3。
- 保留两种 CQE 模式。简化模式仍是每逻辑 QP 最多一个未取走的 signaled marker，
  本次没有扩展成多 marker。直接分发不新增这个限制。
- 双方必须同开关、同 CQE 简化配置、同时间戳配置。响应长度、ABI、控制页
  能力不匹配时拒绝初始化；控制槽保持 512B，token 保持 48 位序号＋16 位用户号。

## 状态机

1. `ACTIVE → WAIT_READY`：非阻塞尝试旧 `db_owner`；关闭用户 DB，原子设置
   `resv_idx` 的冻结位，截取共享 `db_tail=db_stop` 和 `freeze_end`，释放 owner。
   用户 CAS 预留与冻结有明确先后：成功的预留纳入冻结快照；失败的重试选路。
2. 每次调度访问最多检查 64 个描述符；从保存游标继续，遇到空槽就返回。
   **不解冻、不取消、不重新做拥塞判定**。已预留生产者仍可填数据并发布 token。
   其他路径和共享 CQ 继续运行，等待超过报警间隔只输出事件。
3. 所有 token 完整后，从 READ/WRITE 数据段累计 64 位 payload 总量。
   `>=10240` 走复制；更小（包括空区间）走旧路排空。
4. 复制：固定旧在途用户集合与每用户依赖终点；先在备用 SQ 预留整个目标前缀，
   再发布路由版本，新用户只在前缀之后预写。按“无依赖用户、依赖用户”两遍
   稳定复制（每次最多检查 64 个），修正物理 QPN/counter，记录原路径/原序号。
   同用户顺序不变；复制完成前目标禁止 DB。完成后内核只 DB 依赖已解除的
   连续前缀；旧已 DB 区间硬件完成后才开放目标用户 DB。
5. 不复制：新路径可预写，但用户/内核均不能 DB。调度环为该活动槽继续返回
   旧源，发送冻结区间；确认旧路全部硬件完成后再开放目标。
6. 旧路径不因路由已切换就释放。迁移完成映射全部消费后，把源上迁出的槽
   改成 NOP，末尾 NOP 请求 CQE，实际 DB 并等待硬件确认，才回到备用池。
   这是为了防止复用 SQ 时覆盖硬件尚未读取的迁出槽，不传输应用 payload。
   这笔维护 DB 占用/归还 **一个**内部 CQ 信用；不生成用户 WC、不重复归还
   迁出应用请求的信用、不计入提交/完成 payload 字节。

运行时关闭检测不取消进行中的事务。5 秒用户投递等待上限保持基线行为；
永久缺槽不自动取消迁移，需停止应用后重建环境。

## 完成与清理

- 物理 CQ 游标与原请求逻辑完成前缀分离，不能跳过尚未完成的迁移请求。
- 直接分发：先还原原 KQP/序号，按原 token 找用户 CQ，复制 CQE 并恢复原
  QPN/counter；用户用原路径本地 metadata 找回 `wr_id`。CQE 复制完成后发布回收水位。
- 简化：迁移完成置原区间位图、推进连续逻辑前缀；另外发布按用户号索引的
  原路径＋完整序号＋状态记录，独立用户不用等待其他用户的前缀。正常请求仍用水位。
  `MLX5_SRM_CQE_PUBLISH_BATCH` 仍控制水位批量发布；错误和短批次都会刷新。
- 迁移时间戳在目的地继续计 DB 时间，完成时间戳写回原请求；WC 错误也映射回原请求。
- 非法/重复 CQE 不伪装成正常推进；真实传输错误使整个组停止新预留/DB，保留映射
  处理随后硬件 flush CQE，不能把 ERR QP 放回备用池。此版本不自动恢复传输错误，
  应用须在错误后停止并重建；不承诺继续使用出错的逻辑连接。
- QP 销毁先移除 CQ 路由，再将用户 ID 延迟回收，直到所捕获的四路径预留前缀
  都完成且调度器经过静止点，避免旧迁移 CQE 命中新复用 ID。每轮回收预算 256，
  最多每 10ms 检查一次。长久未完成的 ID 留到模块生命周期结束，不能强行复用。
- 模块停止先拒绝新 attach，再停 CQ/调度线程、关门禁，最后释放迁移元数据。
  与普通 verbs 一样，不允许应用一边 post/poll 一边销毁其 QP/卸载模块。

## 运行参数（仅编译开启时存在）

| `/sys/module/mlx5_ib/parameters/` 下参数 | 默认 | 含义 |
|---|---:|---|
| `srm_reroute_enable` | 0 | 是否发起新的自动换路 |
| `srm_reroute_interval_ms` | 10 | 检测周期 |
| `srm_reroute_ratio_gap` | 200 | 两活动路径完成/提交字节比例之差，千分比 |
| `srm_reroute_bad_windows` | 3 | 严格连续的差窗口数；空窗、冷却、差距消失会清零 |
| `srm_reroute_cooldown_ms` | 1000 | 完成事务后的冷却 |
| `srm_reroute_copy_threshold_bytes` | 10240 | 冻结区间的总 payload 阈值 |
| `srm_reroute_warn_ms` | 1000 | 空槽/排空等待报警间隔 |
| `srm_reroute_test_source` | -1 | 指定物理活动路径号的一次性强制触发；-1 关闭 |

计数绑定真实 DB/完成，预写和复制不计字节；检测关闭仍保留路径池和计数，
可作为“相同资源、不换路”的对照。这里的换路是切换 KQP/SQ，不是切 HCA、GID、
端口或保证交换机选择另一条物理网络路径。

经确认的独占测试窗口中，可执行（本次尚未执行这些写操作）：

```bash
echo 1 | sudo tee /sys/module/mlx5_ib/parameters/srm_reroute_enable
# 定向测试不依赖自动拥塞检测：指定初始化日志中的一个活动物理路径号
echo 0 | sudo tee /sys/module/mlx5_ib/parameters/srm_reroute_test_source
sudo dmesg -w | grep --line-buffered SRM_REROUTE
```

事件包括 `freeze`、`wait-hole`、`classify-copy/drain`、`prefill-open`、
`copy-complete`、`wait-old-cqe`、`new-db-open`、`retire-nop-db`、
`wait-retire-cqe`、`recycled`、`fatal-cqe`。含组、源/目标、冻结边界、WQE 数、
总字节与耗时；不会逐 WQE 打印。

## 构建（不安装、不切换运行环境）

在已经配置过当前内核的本仓库中：

```bash
reroute_kernel_dir=$(pwd -P)
make -j8 kernel \
  CWD="$reroute_kernel_dir" \
  AUTOCONF_H="$reroute_kernel_dir/include/generated/autoconf.h" \
  WITH_MAKE_PARAMS=KBUILD_NOCMDDEP=0 \
  'KCFLAGS=-DMLX5_SRM_ENABLE_REROUTE=1 -DMLX5_SRM_ENABLE_CQE_SIMPLIFY=1'
```

`WITH_MAKE_PARAMS=KBUILD_NOCMDDEP=0` 恢复 Kbuild 对编译命令变化的检测。
本仓库 makefile 原本强制 `KBUILD_NOCMDDEP=1`，只改 `KCFLAGS` 可能复用另一
配置的对象文件。没有修改 makefile，也没有移植旧分支的构建选项。
此外只给 `conn.c` 补了 `in_aton()` 所需的 `<linux/inet.h>` 头文件。

关闭构建将 REROUTE 改为 0；直接分发将 SIMPLIFY 改为 0。
不要同时在同一内核工作区编译不同配置。不要直接运行 `kernel_make.sh`：它会安装
并卸载/加载模块，测试窗口确认前不要做。安装、双机换模块另行协调。

配套 rdma-core 的构建见其 `REROUTE_SIZE_AWARE.md`。不需要修改 General 参数，
但运行前必须核实加载的是这一构建的 libibverbs 和 mlx5 provider，而不是系统旧库。

## 测试与当前结论

逐项构建结果和已执行命令见 `REROUTE_VALIDATION.md`。

```bash
bash tests/run_reroute_offline.sh
```

测试直接编译生产 `reroute.inc`，使用内存模拟 SQ/CQ，不加载模块。ASan/UBSan 下：
每配置覆盖 240 次阈值迁移＋4800 次空区间重复复用；简化 0/1、发布批量 1/64。
检查 10239/10240/10241B、多 WQE 总量、空槽不取消、64 项预算、owner 忙、
预写门禁、稳定分组、旧依赖解除、错误映射及失败后不复用、严格连续检测、
NOP 回收确认、16/48/63 位边界。配套 ABI 头文件做逐字节比对。

**离线测试不能证明网卡上的顺序/可见性/数据正确性或吞吐。** 还未加载此模块，
也未进行双机 20 轮 READ/WRITE 数据校验和性能测试。

硬件验收需覆盖简化 0/1 × 用户 DB 0/1：

1. 启动前保存双方 dmesg 起点、源码提交、实际加载模块/库路径和所有宏。
2. 先 `srm_reroute_enable=0`，校验 READ/WRITE 内容与每逻辑 QP 的 WR ID/完成数。
3. 开用户延迟 token 测试钩子；暂停一个或多个生产者后强制 source 换路：
   同一 `freeze` 后连续 `wait-hole`，补齐后继续；其他路径必须仍有进度。
4. 构造冻结待发总量 10239/10240/10241、空区间、大于 64 项的复制区间。
   用带用户号/序号的 payload 校验同用户顺序、无漏发/重发；不能只看吞吐不为零。
5. 在目标预写期间验证门禁，交错完成不同用户；核对错误 WC、原 WR ID、
   时间戳、issued/completed 信用及 NOP 不生成用户 CQE。
6. 连续至少 20 次换路/进程重启，测试正常退出、长时间缺槽、关闭 QP/CQ/context；
   先停进程并关闭设备 FD 再卸载模块。
7. 分别测原基线、路径池开启但检测关闭、大总量复制、小总量排空。记录实际
   活动/备用队列数和内存开销，不预先承诺吞吐提升。

硬件验收通过前，这个分支应视为实验实现，不是已验证可替换当前生产实验的版本。
