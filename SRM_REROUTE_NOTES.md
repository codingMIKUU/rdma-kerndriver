# 空心 RC 换路实现笔记（当前 `srm-fast-reroute` 版本）

本文按当前工作区代码的实际行为，说明路径池初始化、用户选路、并发冻结、
WQE 迁移、逐用户解除阻塞、CQE 还原、状态转换，以及换路机制当前仍带来的
性能开销。

涉及的主要代码：

- 内核路径管理和 scheduler：
  `drivers/infiniband/hw/mlx5/scheduler.c`
- 内核用户 QP 到路径池的绑定：
  `drivers/infiniband/hw/mlx5/qp.c`
- 内核共享结构：
  `drivers/infiniband/hw/mlx5/scheduler.h`
- rdma-core 用户发送路径：
  `/root/zxm/rdma-core/providers/mlx5/qp.c`
- rdma-core CQE 路径：
  `/root/zxm/rdma-core/providers/mlx5/cq.c`
- rdma-core 路径 mmap：
  `/root/zxm/rdma-core/providers/mlx5/verbs.c`

## 1. 核心目标

一个用户 RC QP 不再永久绑定到一个物理内核 QP，而是绑定到一个逻辑 IP
对应的 kernel QP 路径池。

当前默认配置为：

```text
num_kqps = 32 个逻辑 IP
srm_paths_per_ip = 4 条物理路径
path0、path1 = 初始 ACTIVE
path2、path3 = 初始 INACTIVE，作为备用路径
```

所有逻辑 IP 仍使用同一个真实 GID，只是在本实验中模拟多个节点/IP。

当前数量关系为：

```text
physical_kqps = num_kqps * paths_per_ip
active_kqps   = num_kqps * min(2, paths_per_ip)
```

因此默认创建 128 个物理 KQP，其中 64 个处于 active 集合。

`num_kqps` 当前是逻辑 IP 数，不再等价于 active KQP 总数。比较新旧版本性能
时必须先对齐 active KQP 数量。

## 2. 用户 QP 如何分配逻辑 IP

用户创建 hollow RC QP 时，内核先从全局 IDA 分配 `usr_rc_id`，然后计算：

```text
hash1 = jhash(dgid)
hash2 = jhash(hash1, usr_rc_id)
logical_ip = hash2 % num_kqps
base_kqp = logical_ip * paths_per_ip
```

代码位于内核 `qp.c` 的 `mlx5_ib_find_srm_path_group()`。

这具有以下性质：

- 相同 GID、相同 `usr_rc_id`、相同 `num_kqps` 会得到相同结果。
- `usr_rc_id` 的分配顺序变化会改变用户 QP 到逻辑 IP 的映射。
- 应用当前串行创建 server SRM QP，有助于保持 ID 分配顺序稳定。
- 用户程序不需要定义每个逻辑 IP 对应多少用户 QP，内核按哈希自然形成分组。

内核将该逻辑 IP 的全部路径信息返回给 rdma-core。rdma-core 为每条路径保存：

```text
共享 kernel SQ mmap 地址
共享 ctrl page
共享 publish token 数组
kernel QPN
SQ 深度
该用户 QP 对应的 wrid/wqe_head/wr_data 元数据
```

应用仍调用普通的 `ibv_post_send()` 和 `ibv_poll_cq()`，不直接感知换路。

## 3. 每条路径的三个核心索引

每个物理 KQP 都有独立的：

```text
resv_idx       用户已预留到的位置
sched_post_idx scheduler 下一次准备 doorbell 的位置
cons_idx       已完成并可重新使用的位置
```

正常情况下：

```text
cons_idx <= sched_post_idx <= resv_idx
```

三个区间分别表示：

```text
[cons_idx, sched_post_idx)
    已 doorbell，但 completion 尚未全部回收

[sched_post_idx, resv_idx)
    用户已获得 slot，可能正在写或已经写完，但尚未 doorbell

[resv_idx, cons_idx + capacity)
    尚可预留的空间
```

`cons_idx` 不是“用户已经写好的 WQE 索引”。用户是否写完由 publish token 的
sequence 判断。`cons_idx` 表示 completion 已经推进到的位置。

`resv_idx` 和 `cons_idx` 分别占用独立 cacheline，减少用户 reserve 与内核
completion 更新之间的 cacheline 竞争。

## 4. 当前正常选路原则

旧版本使用 `srm_affinity_state` 维护 path 和 outstanding count，并在 post 和
CQE 路径上执行 CAS。当前版本已经完全删除该方案。

现在每个用户 QP 通过以下哈希固定选择 `active_path[0]` 或
`active_path[1]`：

```text
active_slot = hash(usr_rc_cnt, logical_ip) & 1
```

该 `active_slot` 在用户 QP 生命周期内不变。

rdma-core 为每个用户 QP缓存：

```text
srm_pinned_path
srm_route_generation
```

每次 post 时：

1. acquire 读取逻辑 IP 控制页中的 `route_generation`。
2. 如果 generation 没变化，继续使用 `srm_pinned_path`。
3. 如果 generation 变化，根据固定 `active_slot` 重新读取
   `active_path[active_slot]`。
4. 如果该 slot 无效，尝试另一个 active slot。

因此正常运行时，一个用户 QP 始终固定在同一条 active path 上。它不会在每个
请求完成后重新随机选路，也不需要 CQE 上的 affinity count--。

这个规则通过“固定用户 QP 到固定 path”避免跨 kernel RC QP 乱序。换路时，
只有原来属于被替换 active slot 的用户 QP会改变 path。

## 5. 用户态 reserve 和 FROZEN 同步

当前正常 reserve 使用：

```text
load resv_idx
检查 FROZEN
load cons_idx
检查占用是否低于 SQ 深度的 2/3
atomic_fetch_add(resv_idx, 1)
再次检查 fetch_add 返回值中的 FROZEN
```

常态仍只有一次共享 `resv_idx` 原子 RMW，即 `fetch_add`。之前为了换路引入的
常态 CAS 循环已经回滚。

第二次 FROZEN 检查用于处理以下竞争：

```text
用户第一次 load 看见未冻结
内核随后冻结 resv_idx
用户再执行 fetch_add
```

如果 fetch_add 返回值带 FROZEN，用户立即 `fetch_sub` 回滚预留并返回
`EAGAIN`，不会开始填写该 slot。

冻结竞争最终只有两种结果：

```text
用户 fetch_add 先完成
    内核冻结 CAS 失败并重试
    该 WQE 被纳入之后的冻结快照

内核冻结 CAS 先完成
    用户 fetch_add 看到 FROZEN 并回滚
    用户等待 replacement 或源路径解冻
```

因此不存在“用户检查未冻结后，内核冻结却漏掉该 WQE”的窗口。

## 6. 用户发布 WQE

用户在选定路径的共享 kernel SQ 中填写 WQE，随后 release store 一个 64 位
publish token：

```text
[63:32] sequence
[31:16] payload bytes，64 B 粒度
[15:0]  usr_rc_cnt
```

scheduler acquire load token。只有：

```text
token.sequence == post_idx + 1
```

才认为 WQE 已完整发布。

payload bytes 当前由 rdma-core 的 `mlx5_srm_wr_data_bytes()` 再次遍历 SGE
计算，用于 `SCHED_SIZE` 和换路 detector 的 byte counter。

## 7. 换路 detector

detector 不是单独线程。它嵌在 scheduler 线程的外层循环中，默认每 10 ms
执行一次。

每个逻辑 IP 读取当前两个 active path，并计算本窗口：

```text
posted    = posted_bytes - prev_posted_bytes
completed = completed_bytes - prev_completed_bytes
ratio     = min(1000, completed * 1000 / posted)
inflight_growth = max(0, inflight - prev_inflight)
```

当前 `efficiency_bad` 的真实条件是：

```text
poor 本窗口 posted > 0
AND good 本窗口 posted > 0
AND good.ratio - poor.ratio >= srm_reroute_ratio_gap
```

除此以外还要求：

```text
两个 active path 不相同且都存在
AND 两条 path 当前都没有 migrate_peer
AND poor 已经过 cooldown
AND 存在可用 replacement
AND bad_windows 达到 consecutive_windows
```

当前默认参数：

```text
srm_reroute_interval_ms = 10
srm_reroute_ratio_gap = 200          # 20 个百分点
srm_reroute_consecutive_windows = 3
srm_reroute_cooldown_ms = 1000
```

注意：`bad_windows` 保存在物理 path 上。如果下一窗口另一个 path 变成 poor，
原 poor 的计数不会被主动清零。因此当前语义接近“同一物理 path 累计达到三个
bad window”，并不严格保证是连续三个相邻检测窗口。

当 `paths_per_ip == 2` 时没有独立 spare，代码会把另一条 active path 当作
目标。`paths_per_ip > 2` 时从 INACTIVE path 中选择第一个 spare。

## 8. 开始换路和冻结快照

以 `path0 -> path2` 为例，内核首先为迁移元数据做惰性分配：

```text
path2.migrate_meta
path0.migrate_blocked_users
path0.migrate_user_inflight
```

随后内核 CAS 设置：

```text
path0.resv_idx |= FROZEN
```

冻结成功后记录：

```text
db_stop_idx     = path0.sched_post_idx
migrate_end_idx = 冻结前的 path0.resv_idx
move_nr         = migrate_end_idx - db_stop_idx
```

待迁移范围为：

```text
[db_stop_idx, migrate_end_idx)
```

这是用户已经预留但内核尚未 doorbell 的 WQE。

## 9. 迁移专用 inflight 快照

旧版本在每个正常 WQE/CQE 上维护：

```text
route->path_inflight[usr_rc_cnt][path]
```

当前版本已经删除该常态计数。

冻结成功后，scheduler 只执行一次快照：

```text
扫描 [cons_idx, db_stop_idx)
解析每个 publish token 的 usr_rc_cnt
migrate_user_inflight[usr_rc_cnt]++
```

该区间正是旧 path 上已经 doorbell、但 completion 尚未回收的 WQE。

迁移期间，只有旧 source path 的 CQE 才会：

```text
migrate_user_inflight[usr_rc_cnt]--
```

因此未发生换路时：

- 发送 WQE 不再更新 per-user path inflight。
- 完成 CQE 不再更新 per-user path inflight。
- `usr_rc_routes[]` 中也不再保存 `path_inflight[]`。

## 10. 迁移启动失败和 FREEZE_REQ

源路径冻结后，迁移仍可能因为以下原因失败：

```text
旧 inflight token 尚不合法
待迁移 token 尚未 ready
WQE 不是单 BB
目标 SQ 不为空或空间不足
目标 resv_idx 已冻结
```

这时不能直接无条件清除 FROZEN，因为用户可能正在执行回滚。代码将源状态设置
为：

```text
FREEZE_REQ
```

active_path 尚未修改，所以源路径仍在 active 扫描集合中。

scheduler 之后调用 `mlx5_srm_progress_abort_freeze()`，只在：

```text
resv_idx == migrate_end_idx | FROZEN
```

时通过精确 `cmpxchg` 清除 FROZEN，并把状态恢复为 ACTIVE。

因此 `FREEZE_REQ` 是“迁移启动失败后，等待安全解冻”的真实过渡态，不是未使用
状态。

## 11. 目标区间预留和 replacement 发布

迁移继续前，内核在目标 path 上一次性预留 `move_nr` 个 slot。

独立 INACTIVE spare 必须满足：

```text
dst_resv == dst_cons == dst.sched_post_idx
```

目标加入迁移 WQE 后的占用还必须小于 SQ 深度的 `2/3`。

预留成功后建立 source/destination peer，并依次发布：

```text
source.replacement_path = destination.path_idx
source.state = COPYING
destination.state = COPYING
active_path[被替换的 slot] = destination.path_idx
route_generation++
```

`active_path` 先发布，`route_generation` 后发布。用户 acquire 看到新 generation
后，可以看到对应的新 active path。

在 source 已冻结但 replacement 尚未发布的短窗口中，用户 reserve 返回
`EAGAIN`，反复检查：

1. 如果 `replacement_path` 有效，切换到 replacement。
2. 如果 source 已因失败而解冻，重新读取 pinned path。
3. 否则按退避参数等待，直到超时。

## 12. 搬迁 WQE

冻结只阻止新的 slot 预留，不阻止冻结前已获得 slot 的用户继续填写。因此内核
必须先确认：

```text
[db_stop_idx, migrate_end_idx)
```

中的所有 publish token 都 ready。

当前迁移只支持单个 64 B BB 的 WQE。signature QP 或多 BB WQE返回
`-EOPNOTSUPP`。

复制每个 WQE 时：

```text
复制 64 B WQE
把 WQE counter 改为 destination post_idx
把 QPN 改为 destination kernel QPN
重新生成 destination publish token
记录 source_qpn/source_post_idx/dest_post_idx
```

源 SQ 中对应的未 doorbell WQE 被改成 NOP，防止该物理 SQ以后继续推进时重复
执行旧操作。

## 13. 逐用户解除阻塞

若用户 QP A 在 source 上还有已 doorbell、未完成的 WQE，那么 A 的迁移 WQE
不能立即在 destination 上执行，否则同一用户 RC QP可能跨两个 kernel RC QP
乱序。

内核按 `migrate_user_inflight[A]` 将待迁移 WQE分成两类：

```text
unblocked：该用户在 source 上没有旧 inflight
blocked：  该用户在 source 上仍有旧 inflight
```

复制顺序为：

```text
先复制全部 unblocked WQE
再复制全部 blocked WQE
```

不同用户 RC QP之间可以重排。对同一用户 QP，其待迁移 WQE属于同一类别，并且
类别内部保持原顺序。

`destination.migration_ready` 初始指向 unblocked 前缀末尾。

destination 处于 DRAINING 时，scheduler 只允许：

```text
sched_post_idx < migration_ready
```

范围内的 WQE进入 doorbell。如果到达被阻塞位置，scheduler 直接跳过该 KQP，
继续服务其他 active KQP，不会原地等待，也不会阻塞整个 scheduler。

随着 source CQE 到达，迁移专用计数递减。每轮扫描 destination 时，
`mlx5_srm_advance_migration_ready()` 从当前位置向后推进连续可发送前缀。

## 14. active-only scheduler 扫描

旧版本遍历：

```text
num_kqps * paths_per_ip
```

个物理 KQP，包括所有 INACTIVE spare。

当前 scheduler 每轮只读取每个逻辑 IP 的：

```text
active_path[0]
active_path[1]
```

实际扫描数为：

```text
num_kqps * min(2, paths_per_ip)
```

如果两个 active slot 指向同一 path，会去重。

换路成功发布 active_path 后，source 不再属于 active 扫描集合。因此迁移由
active destination 通过 `migrate_peer` 反向推进 source 的 COPYING/DRAINING
状态。

detector 仍会按低频检测周期遍历 physical path，以选择 spare 和更新历史计数；
它不属于 scheduler 每轮的高频扫描。

## 15. 内核 CQE 定位

scheduler doorbell 前写入：

```text
wr_id[63:32] = kqp_idx
wr_id[31:0]  = post_idx_low32
```

单 CQ 配置下不需要编码 CQ index。

poll CQE 时通过 `wr_id` 得到：

```text
kqp_idx -> send_srmc
post_idx -> publish token slot
token -> usr_rc_cnt + payload bytes
usr_rc_cnt -> cqb + uidx
kqp_idx -> ctrl_page
```

正常 CQE 不再依赖发送路径填写 `wqe_infos[]`。

每个 CQE 仍维护：

```text
completed_bytes
inflight_bytes
inflight_wqes
```

但只有 `send_srmc->migrate_source` 为真时，才可能访问和递减
`migrate_user_inflight[]`。

## 16. 迁移 CQE 还原

迁移 WQE 的硬件 CQE携带 destination kernel QPN 和 destination counter。

内核通过 destination slot 的 `migrate_meta` 找到：

```text
source_qpn
source_post_idx
dest_post_idx
```

向用户 CQ发布前，将 CQE 的 QPN 和 WQE counter 改回 source 值。

这样 rdma-core 根据 source kernel QPN 找到 source path 的 `wq`，再通过原始
slot 读取用户最初保存的 `wr_id`。

每个迁移 CQE完成后：

```text
migrate_meta.valid = 0
destination.migrated_outstanding--
```

## 17. 迁移完成和状态转换

source 上已 doorbell 的旧 WQE全部完成，即：

```text
source.inflight_wqes == 0
```

之后：

```text
destination -> ACTIVE
destination.migration_ready = migration_end
source.sched_post_idx = source.migrate_end_idx
source.cons_idx = source.migrate_end_idx
source -> QUIESCED
```

source 仍保持 FROZEN，直到 destination 上全部迁移 WQE完成：

```text
destination.migrated_outstanding == 0
```

然后内核精确清除 source 的 FROZEN，并执行：

```text
source -> INACTIVE
source.replacement_path = U8_MAX
source.migrate_source = 0
source.migrate_peer = NULL
destination.migrate_peer = NULL
```

完整成功状态转换为：

```text
source:
ACTIVE -> COPYING -> DRAINING -> QUIESCED -> INACTIVE

destination:
INACTIVE -> COPYING -> DRAINING -> ACTIVE
```

失败解冻路径为：

```text
source:
ACTIVE -> FREEZE_REQ -> ACTIVE
```

完整成功日志通常为：

```text
hollow RC reroute detect
hollow RC reroute start
hollow RC reroute complete
```

## 18. 当前已经消除的稳态开销

相较旧笔记对应的版本，以下成本已经删除：

1. 删除用户 post 上的 affinity count++ CAS。
2. 删除用户 CQE 上的 affinity count-- CAS。
3. 删除 affinity `count == 0` 时的 `rdtsc`。
4. 用户 QP 不再在每个 outstanding 阶段重新选择 path。
5. reserve 常态路径从 CAS 循环恢复为 `atomic_fetch_add`。
6. 删除逐 WQE 的 `route->path_inflight[path]++`。
7. 删除逐 CQE 的 `route->path_inflight[path]--`。
8. scheduler 不再高频扫描 INACTIVE spare。

正常请求现在主要只有一个共享原子 RMW：

```text
post: resv_idx fetch_add
```

## 19. 未发生换路时仍存在的开销

即使没有任何 reroute 日志，以下成本仍然存在。

### 19.1 active KQP 数量变化

当前：

```text
active_kqps = num_kqps * min(2, paths_per_ip)
```

例如新旧版本都写 `num_kqps = 256` 时：

```text
旧版 active KQP = 256
新版 paths_per_ip=4 时 active KQP = 512
```

这会增加扫描和 CQ polling 次数，并把流量分散到更多 SQ，降低平均 DB batch。
这是当前最需要对齐后再比较的结构性差异。

### 19.2 每个 post 读取 route_generation

每个 WR 都会从共享 route ctrl page acquire load `route_generation`，然后读取本地
pinned path。generation 不变时不会读取两个 active_path，但仍多一次共享控制页
访问和路径结构间接寻址。

### 19.3 reserve 的 FROZEN 检查

每个 reserve 都会在 fetch_add 前检查一次 FROZEN，并检查 fetch_add 返回值。
它不增加第二个正常原子 RMW，但增加了分支和异常回滚代码。

### 19.4 两个 active path 拆分聚合

用户 QP通过稳定哈希分布到两个 active slot。与每个逻辑 IP 只有一个 KQP相比：

- 单个 KQP收到的 WQE变少。
- 平均 DB batch 可能下降。
- doorbell 次数可能增加。
- SQ、token 和 ctrl page 工作集扩大。

### 19.5 rdma-core 每个 CQE 线性查找 path

正常 CQE根据 kernel QPN 遍历：

```text
for path in srm_paths:
    if path.kernel_qpn == cqe.qpn:
        use path.wq
```

默认最多比较四条 path。该操作位于逐 CQE 热路径，在小消息高 PPS 场景下可能
非常显著。

### 19.6 payload bytes 再次遍历 SGE

构建 WQE已经遍历 SGE，发布 token 时 `mlx5_srm_wr_data_bytes()` 又遍历一次。
单 SGE成本较小，但会按每 WQE重复。

### 19.7 每个 WQE/CQE 的 detector 统计

发送每个 WQE仍更新：

```text
posted_bytes
inflight_bytes
inflight_wqes
```

完成每个 CQE仍更新：

```text
completed_bytes
inflight_bytes
inflight_wqes
```

它们不再包含 per-user `path_inflight`，但仍是逐 WQE/CQE 写操作。

### 19.8 active-only 扫描本身的索引开销

当前每个 active scan 仍需要：

```text
scan / active_paths
scan % active_paths
logical_ip * path_count
读取 route_ctrl->active_path[]
根据 physical index 查 srmc
```

它比扫描所有 spare 好很多，但还不是紧凑 active 指针数组。

### 19.9 周期 detector

默认每 10 ms：

- 遍历每个逻辑 IP 的两个 active path。
- 做 byte delta、比例和 inflight growth 计算。
- 再遍历全部 physical path 更新 previous counters。

关闭 `srm_reroute_enable` 会消除 detector 调用，但不会消除多路径 post、CQE 和
active KQP 拓扑成本。

### 19.10 调度状态分支和让出 CPU

每个 active KQP仍检查 migration state/peer。scheduler 每
`MLX5_SRM_RESCHED_SCAN_INTERVAL` 个外层 round 调用一次 `cond_resched()`，用于
避免长期运行导致 soft lockup。

## 20. 真正发生换路时的额外成本

发生换路后还会增加：

1. source `resv_idx` 冻结 cmpxchg。
2. 冻结窗口内用户 EAGAIN、replacement 检查和退避。
3. 惰性分配 migration metadata、bitmap 和 per-user inflight 数组。
4. 扫描 `[cons_idx, db_stop_idx)` 构造 per-user inflight 快照。
5. 扫描待迁移范围并验证 token 和 WQE DS。
6. 目标 SQ 区间预留 cmpxchg。
7. 两遍分类和复制待迁移 WQE。
8. 每个 WQE 复制 64 B，修改 counter/QPN/token。
9. 每个迁移 WQE 写 `migrate_meta`。
10. 将 source 未 doorbell WQE改成 NOP。
11. destination 每轮推进 `migration_ready`。
12. source 迁移期 CQE递减迁移专用 per-user counter。
13. 迁移 CQE还原 source QPN/counter。
14. 等待 source 旧 inflight 清零。
15. 等待 destination migrated completion 全部完成后才能复用 source。

因此真实换路会带来短时吞吐下降和尾延迟上升，主要来源是冻结等待、内存扫描、
WQE复制、旧 inflight 排空和 cache 扰动。

## 21. 参数配置

核心模块参数：

```text
srm_paths_per_ip                 默认 4，只能加载模块时设置
srm_reroute_enable               默认 1
srm_reroute_interval_ms          默认 10
srm_reroute_ratio_gap            默认 200
srm_reroute_consecutive_windows  默认 3
srm_reroute_cooldown_ms          默认 1000
```

例如：

```bash
sudo modprobe mlx5_ib \
    srm_paths_per_ip=4 \
    srm_reroute_enable=1 \
    srm_reroute_interval_ms=10 \
    srm_reroute_ratio_gap=200 \
    srm_reroute_consecutive_windows=3 \
    srm_reroute_cooldown_ms=1000
```

运行时可关闭 detector：

```bash
echo 0 | sudo tee /sys/module/mlx5_ib/parameters/srm_reroute_enable
```

但这不是旧版性能基线。接近单路径基线需要重载：

```bash
sudo modprobe mlx5_ib srm_paths_per_ip=1 srm_reroute_enable=0
```

## 22. 当前建议的性能优化顺序

1. 对齐新旧版本 active KQP 数量后再比较吞吐。
2. CQE 先检查 `srm_pinned_path`，失败时才线性扫描全部 path。
3. scheduler 维护紧凑 active KQP 指针/索引数组，换路时只替换一个元素。
4. 将 posted/completed/inflight 统计改成每 DB/CQ batch 聚合更新。
5. 在现有 WQE SGE 构建循环中顺便累计 bytes，删除第二次 SGE遍历。
6. 评估是否能把 route generation 检查与 reserve 控制状态合并，进一步减少
   每个 post 的共享控制页访问。

## 23. 当前结论

当前版本已经去掉了旧方案最重的 affinity CAS 和常态 per-user path inflight
维护，并把 scheduler 从扫描所有 physical KQP 收缩为只扫描 active KQP。

现在“没有发生换路但吞吐仍下降”的主要嫌疑依次是：

1. 相同 `num_kqps` 下 active KQP 数翻倍，导致 DB batch 被拆散。
2. rdma-core 每个 CQE线性查找 path。
3. 每个 post 的 route generation 读取和多路径间接寻址。
4. 每 WQE/CQE 的 byte/inflight detector 统计。
5. active-only 扫描仍有动态索引和 route ctrl 访问。
6. payload bytes 的第二次 SGE遍历。
7. 低频 detector 和周期 `cond_resched()`。

`srm_reroute_enable=0` 只关闭 detector 和迁移动作，不会关闭前六项稳态成本。
