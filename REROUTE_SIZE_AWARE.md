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
2. 每次调度访问共用 64 项描述符处理预算（完整性扫描、依赖快照、两遍复制、
   NOP 回收合计）；从保存游标继续，遇到空槽就返回。
   **不解冻、不取消、不重新做拥塞判定**。已预留生产者仍可填数据并发布 token。
   其他路径和共享 CQ 继续运行，等待超过报警间隔只输出事件。
3. 所有 token 完整后，从 READ/WRITE 数据段累计 64 位 payload 总量。
   非空区间且 `bytes >= srm_reroute_copy_threshold_bytes` 走复制；更小或空区间
   走旧路排空。**阈值设成 1 也不会搬运空区间**，旧路已经 DB 的请求不计入
   待搬运 bytes；新路径后来预写的 WQE 也不参与本次分类。
4. 复制：固定旧在途用户集合与每用户依赖终点；先在备用 SQ 预留整个目标前缀，
   再发布路由版本，新用户只在前缀之后预写。按“无依赖用户、依赖用户”两遍
   稳定复制（计入同一次访问的共享预算），修正物理 QPN/counter，记录原路径/原序号。
   同用户顺序不变；复制完成前目标禁止 DB。完成后内核只 DB 依赖已解除的
   连续前缀；旧已 DB 区间硬件完成后才开放目标用户 DB。
5. 不复制：先扫描旧路径 `[cq_complete_idx, freeze_end)`，同时覆盖已 DB 未完成
   请求和尚未 DB 的冻结后缀，记录每用户最后一个旧请求的完成终点。快照完成后
   发布路由，用户可在目标预写，但不能直接 DB。调度环先服务旧源，直到
   `db_tail == freeze_end`（全部旧请求已提交，并非全部完成），再切换到目标，
   输出 `drain-ready` 并结束本次计时。内核按每个 WQE 的用户编号检查旧依赖，
   发送完整且依赖已解除的连续前缀。没有旧依赖的用户不必等待其他用户排空；
   队头仍有依赖时不能跳过它发送后面的 WQE。旧源的独立 CQ 继续由已有入队逻辑
   轮询，不能因 DB 调度转向目标而漏掉旧完成。旧路全部硬件完成后输出
   `new-db-open`，恢复用户直接 DB，再回收旧路。未改变 copy 的提前 DB 范围。
6. 旧路径不因路由已切换就释放。迁移完成映射全部消费后，把源上迁出的槽
   改成 NOP，末尾 NOP 请求 CQE，实际 DB 并等待硬件确认，才回到备用池。
   这是为了防止复用 SQ 时覆盖硬件尚未读取的迁出槽，不传输应用 payload。
   这笔维护 DB 占用/归还 **一个**内部 CQ 信用；不生成用户 WC、不重复归还
   迁出应用请求的信用、不计入提交/完成 payload 字节。

运行时关闭检测不取消进行中的事务。5 秒用户投递等待上限保持基线行为；
永久缺槽不自动取消迁移，需停止应用后重建环境。

## 每物理 KQP 的在途 payload 上限

字节窗口独立于换路编译开关和 worker `limit_batch`，关闭换路也生效。例如设置 32KiB：

```c
/* include/uapi/rdma/mlx5-srm-reroute.h */
#define MLX5_SRM_MAX_INFLIGHT_BYTES 32768U
```

32KiB 是**每个物理 KQP 已 DB、尚未被内核确认硬件完成的 payload 总和**，
不是每次 DB 的大小，也不是一个逻辑组的四条路径共用额度。READ 按读取长度、
WRITE 按写入长度计数，不含协议头。设为 0 关闭此实验限制。内核初始化控制页
时发布实际值，用户驱动读取该值，避免两个驱动宏值不同造成超发。

- 用户、内核均在取得同一个 `db_owner` 后，计算
  `available = limit - (posted_bytes - completed_bytes)`，不足时返回调度/
  投递路径，不忙等。扫描只能选择总 payload 不超过 available 的连续前缀；
  stride 用户仍须满足原来的“到自身全部可 DB”规则，否则交给内核。
- **drain 旧源例外**：路径处于 `MLX5_SRM_ROUTE_DRAIN` 时，内核 DB 不受上述
  在途字节窗口限制，可将冻结的旧区间继续发出，不必为字节额度等待 CQ 完成。
  用户 DB 仍关闭；完整性、`freeze_end` 边界、worker 信用（`limit_batch`）和 CQ
  容量等检查不变，其他限制导致一次发不完时仍由后续正常调度继续。
  新目标及其他活动 QP 仍受字节窗口限制。旧源允许暂时超过字节上限，但实际 DB
  和 CQ 完成继续准确记账；旧请求完成并回收后，恢复正常字节额度限制。
- 预留、填 WQE、复制不扣字节额度。实际 doorbell 才累计 posted_bytes；
  worker 授信缩小前缀时也相应缩小字节记账。CQ 完成归还**物理目标 QP**的额度，
  不跟随逻辑完成映射重复归还源 QP；维护 NOP 不计应用 payload。
- 完成不依赖应用取走 WC。关闭自动检测 `srm_reroute_enable=0` 时仍归还额度。
  独立 CQ 的空闲态批量完成仍一次发布整批 completed_bytes；迁移/错误处理沿用
  逐项完成路径。累计字节按 64 位无符号回绕计算，路径复用不清零累计值。
- 单个 WR 大于非零上限时，不再因字节窗口返回 `EMSGSIZE`，也不拆分 WR。
  正常受限路径只有在该物理 QP 的在途 payload 为 0、且当前 DB 尚未选入任何
  WQE 时，才允许将这个超限 WR **单独 DB**。其完整 payload 仍记入 posted_bytes；
  后续 WQE 等它完成、额度归还后再发送。不是把窗口扩大成可容纳多条的批量预算。
  比如 32KiB 窗口可以单独发送一个 64KiB WR，但不能同时再发送另一个 WR。
  小于等于上限但超过当前剩余额度的 WR 仍需等待，不适用单条超限例外。
  drain 旧源继续完全豁免字节窗口，不受这个 singleton 规则限制。
- `MLX5_SRM_ENABLE_REROUTE=0` 同样限制用户与内核 DB。共享 CQ、独立 CQ、
  CQE 简化开/关均在发布可复用 SQ 水位前归还已完成区间的字节额度。
  独立 CQ 保留每轮一次水位发布，但非零字节窗口需要读取已完成 WQE 的长度来记账。
  上限设为 0 时，非换路内核不编译这些字节记账；用户跳过字节计数读写和长度累加。
  原有 WQE 信用、SQ 深度、DB 上限、完成/依赖门禁同时生效，普通 RC 不受此限制。
- 使用未 signaled 的 WR 时，应用必须保证字节窗口耗尽前已有可产生完成通知的
  请求；否则无法及时归还字节信用。当前每 WQE 请求 CQE 的实验不涉及此问题。

这会增加冻结时存在未 DB WQE 的可能性，**不保证发生 copy**。当前阈值 1B 下，
非空冻结区间走 copy；空区间仍走 drain。不能把字节限流当作强制换路测试钩子。
限流本身可能降低吞吐，并增加共享完成字节计数发布，性能需实测。

控制槽仍是 512B，换路 ABI 保持 `0x52520103`（再叠加 CQE 模式位）。
另增加 QP 创建能力位和控制页能力位，避免旧用户库不记账或旧模块不归还信用。
新用户库拒绝没有该能力的模块；启用非零上限的新模块拒绝旧用户库创建空心发送 QP。
因此**此次需同时重新构建用户驱动和内核驱动**。以后只调此内核上限、不改接口，
用户驱动从控制页读取新值。内核仍可使用原 `sudo bash kernel_make.sh`，
不要在实验运行中卸载模块；本次验证没有安装或重载模块。

字节窗口初始化日志（示例为 4MiB 上限、关闭换路；不需要开启统计）：
```text
SRM byte window: reroute=0 per_physical_kqp=4194304 bytes (0=unlimited) oversized_wr=single_when_empty
```

换路初始化日志：
```text
SRM reroute byte window: per_physical_kqp=32768 bytes (0=unlimited), oversized_wr=single_when_empty, drain_source=unlimited
```

## 连续阶段推进与处理段计时

可立即完成的阶段不再强制返回调度环：例如 READY → SNAPSHOT → RESERVE →
COPY0 → COPY1 可在同一次 `rr_step()` 内连续执行。只有缺槽、预算不足、owner
忙、空间不足或等待硬件完成等真实阻塞才返回；下一次从原游标继续。检测新发起
事务后，同次 `rr_schedule()` 就调用 `rr_step()`，不额外等待下次访问。
copy 的依赖快照终点是 `db_stop`；drain 的终点是 `freeze_end`。两者都使用
每用户依赖位图与终点，不能漏掉 drain 中仍需留在旧路发送的请求。

`SRM_REROUTE_TIMING` 新增 `timing_scope=reroute_work_tsc`，原三个字段现在是
**分段累计的状态机处理 cycles，而不是端到端等待 cycles**：

- `switch_to_user_prewrite_cycles`：取得源 owner 后冻结，以及后续实际执行的
  检查、分类、依赖快照、目标预留、路由发布，到允许用户预写。
- `user_prewrite_to_kernel_db_cycles`：从预写开放后，实际执行的复制/检查/门禁
  推进。copy 截止于 `copy_ready`；drain 现在截止于 `drain_ready`，即依赖已建立、
  旧区间已 DB，目标开始允许内核按依赖检查发送。此时可能全部新请求都仍有依赖，
  因此该终点不是“已完成一次目标 doorbell”或“所有新请求已经能发送”。
- `total_cycles`：上述两项之和。不包含之后的源路径 NOP 回收。
- `blocked_undb_avg_cycles`：`total_cycles / blocked_undb_wqes`，新标识为
  `blocked_undb_avg_scope=reroute_work_per_source_undb_wqe`。保留原字段名，
  分母统一为**冻结时源 QP 已预留、尚未 DB 的 WQE 数**，即
  `mlx5_srm_rr_seq(freeze_end - db_stop)`，
  `blocked_undb_scope=source_frozen_undb_wqes`。copy 与 drain 使用相同口径：
  copy 统计被搬运的源区间；drain 统计留在旧路径继续发送的同一区间。
  使用冻结时固定边界、支持序号回绕，后续旧 DB 推进不会使计数缩小。
  不包含用户在新 QP 上预留/预写的 WQE，也不含冻结前源 QP 已 DB 的请求。
  只有冻结区间为空时，才输出 count=0、avg_cycles=0、avg_valid=0。
  这是**换路处理总开销按冻结的源端未 DB WQE 数摊销**，不是单条 WQE 的真实
  等待时间，也不是仅 memcpy 的平均成本；原两段 cycles、total 和计时终点均不变。

每次退出状态机即停止累计，下次进入再继续；事件日志前暂停、日志后恢复。
阶段间调度等待、等待 CQ 到达的时间、普通 CQ/DB 工作、检测和 printk 均不
计入。copy-ready / drain-ready 之后，即使多次检查旧完成、依赖、owner 或执行
回收，也不再累计或重复输出本次 TIMING。这不是整个换路生命周期所有辅助
CQ/DB 路径的 CPU profiler；drain 等待旧区间实际 DB 时，状态机的重试检查计入，
正常调度器的扫描/doorbell 本身仍在计时区间之外。
为避免影响运行，不关闭抢占/中断；处理段内部发生的中断或被抢占仍会污染
TSC 差值，因此它也不是严格排除中断的硬件任务周期数。

此前的 drain 依赖/提前 DB 改动只涉及内核；本次新增字节窗口另有配套用户驱动和
ABI 改动，不能再仅重编内核。drain 依赖和提前 DB 策略保持不变。

离线验证增加：同次访问连续推进、阶段边界共享预算、目标 owner 忙时返回、
copy/drain 计时求和，以及注入每条日志/每次调度间隔 10 亿假 cycles 后处理段
计时不变。drain 测试另覆盖已发/未发旧请求依赖、同用户最后终点、部分 DB 时
不开放、独立请求提前完成、连续前缀门禁、未发布 token、提前结束计时、目标
owner 冲突、源错误关闭路径和 16/48/63 位回绕；阈值 1 下分别检查空/非空区间。
真实设备性能和中断噪声需重装后另测。

## 完成与清理

- 普通成功 CQE 在整个组 `RR_IDLE`（无迁移映射、缺口或待回收 NOP）时走快速
  路径，不查询迁移表/位图/token；错误始终走完整路径。不能仅凭目标 `ACTIVE`
  或运行时检测关闭来判断，因为目标开放 DB 后仍可能有迁移请求未完成。
- 完成字节数仍在内核私有每路径状态累加；启用字节窗口时同时发布共享
  `route.completed_bytes`，供用户 DB 计算额度。物理完成位置由
  `cq_complete_idx` 维护，`route.physical_cons` 仍保留不用。用户完成检测
  继续使用原来的 `cons_idx`；不能把字节计数当成 WQE 完成水位。
- 直接分发复用换路解析得到的原 KQP 和完整序号，用户 token 只在原 CQ 路由
  校验中读取一次；简化模式只为迁移/错误记录读取 token。
- 内核在现有完整性扫描中累计 payload，领取信用后只计入实际 DB 的前缀。
  完整授信无二次遍历；部分授信时在 DB 前按较短的有效前缀或截断后缀校正。
  不改变先扫描后领信用、CQ poll 预算、用户 DB、发布 batch 或换路门禁。
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

修改默认迁移阈值：直接编辑 `drivers/infiniband/hw/mlx5/reroute.inc` 中的
`srm_reroute_copy_threshold_bytes = 10UL * 1024`，例如改成 `20UL * 1024`。
单位是字节，比较的是整个冻结待发区间的 payload 总和，不是单个 WQE。
这个值仅由内核使用，不需要同步修改 rdma-core；修改源码后需重新构建、安装并
加载内核模块才生效。编译开启换路且模块已加载时，也可通过同名 sysfs 参数
在线修改；已经完成分类的事务不重新分类，新的值用于后续分类。

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
`copy-complete`、`wait-old-db`、`drain-ready`、`wait-old-cqe`、`new-db-open`、`retire-nop-db`、
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
python3 ../rdma-core/tests/test_srm_reroute_byte_window.py
```

测试直接编译生产 `reroute.inc`，使用内存模拟 SQ/CQ，不加载模块。ASan/UBSan 下：
每配置覆盖 240 次阈值迁移＋4800 次空区间重复复用；简化 0/1、发布批量 1/64。
检查 10239/10240/10241B、多 WQE 总量、空槽不取消、64 项预算、owner 忙、
预写门禁、稳定分组、旧依赖解除、错误映射及失败后不复用、严格连续检测、
NOP 回收确认、16/48/63 位边界。配套 ABI 头文件做逐字节比对。
字节窗口额外覆盖 0/32768、等于/超过上限、不同 QP 独立额度、部分 worker 授信、
迁移后物理归账、NOP 不计费、64 位累计字节回绕及关闭自动检测后归还额度。
用户侧测试提取生产 DB 函数执行，检查队头/stride、空槽、门禁、额度退还及回绕；
不调用网卡、不修改安装库。内核 DB 的扫描模型与实际 CQ/状态机测试并行覆盖，
不能代替硬件压测。

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

## 普通 RC 创建 CQ 的 Oops 与卸载等待（2026-09-20）

普通 RC 原先也会进入 `mlx5_ib_map_cq_ubuf()`，额外调用一次
`get_user_pages()`。其返回值 `int ret` 与 `size_t npages` 比较时，负错误码
被提升为无符号数，可能绕过失败检查；随后 `vmap` / `put_user_pages`
会访问未取得的页。该调用还缺少 GUP 所要求的 mmap 读锁。
创建 CQ 的 ioctl 内发生 Oops 后，无法正常退出 SRCU 读侧；卸载栈中的
`uverbs_disassociate_api_pre -> synchronize_srcu` 与此故障链吻合。

修复不绕过卸载同步：普通 RC 不再额外映射 CQ。仅 Hollow/SRM 发送 QP
首次绑定 CQ 时，从现有 `ib_umem` 的物理页 SG 列表取得额外页引用并映射。
同一 CQ 的并发绑定由 resize mutex 串行化；失败只释放实际取得的引用。
映射记录保留原始 vmap 地址、页数及用户页内偏移，继续沿用原有延迟回收。
已映射的 Hollow/SRM CQ 暂不支持更换大小，返回 `-EBUSY`；普通 RC CQ
resize 不变。现有软件分发仅支持 64B CQE，不支持的 Hollow CQ 明确拒绝。
不修改 `post_send`、DB、CQ poll 数据路径，不改变用户态 ABI。

无 sudo、无模块安装的验证：

```bash
python3 tests/test_srm_cq_mapping.py
python3 tests/check_srm_kernel_compile.py --units scheduler cq qp ah main --reroute 0 --private-cq 0
python3 tests/check_srm_kernel_compile.py --units scheduler cq qp --reroute 0 --private-cq 1 --latency-cq-priority 1
python3 tests/check_srm_kernel_compile.py --units scheduler cq qp --reroute 1 --private-cq 0
python3 tests/check_srm_kernel_compile.py --units scheduler cq qp --reroute 1 --private-cq 1 --latency-cq-priority 1
bash tests/run_private_cq_offline.sh
bash tests/run_reroute_offline.sh
```

这些检查已通过，但未在网卡上验证本次修复。已发生 Oops 且卸载卡住的内核
需要先重启，不能靠源码修改恢复 SRCU 状态；不要强制卸载或绕过同步。
重启后可沿用 `sudo bash kernel_make.sh` 完整编译、安装并加载修复。
本次修复本身不要求重编 rdma-core / General；仍应保留与原有其他功能匹配的用户驱动。
