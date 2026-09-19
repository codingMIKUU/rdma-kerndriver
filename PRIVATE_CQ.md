# 每 KQP 独立发送 CQ（包括换路、CQE 简化路径）

## 开关和构建

本次没有切换分支、安装、重载模块、改 RDMA-General、修改远端，也没有调整
现有 num_kqps、limit_batch、用户 DB、大小流及统计配置。

独立 CQ 的开关在 `drivers/infiniband/hw/mlx5/scheduler.h`。实际默认值请以当前工作树里的宏为准；以下是启用组合的示例，不会自动修改源码：

```c
#define MLX5_SRM_ENABLE_PRIVATE_CQ 1          /* 1=独立 CQ，0=原共享 CQ */
#define MLX5_SRM_PRIVATE_CQ_POLL_BUDGET 256  /* 每次最多消费多少个 CQE */
```

测试独立 CQ 时必须满足：

- 内核 `scheduler.h` 与用户驱动 `providers/mlx5/mlx5.h` 均为
  `MLX5_SRM_ENABLE_CQE_SIMPLIFY=1`。
- 换路可以关闭或开启编译；若开启，内核
  `include/uapi/rdma/mlx5-srm-reroute.h` 与用户驱动
  `rdma-core/kernel-headers/rdma/mlx5-srm-reroute.h` 的
  `MLX5_SRM_ENABLE_REROUTE=1` 必须一致，且遵守换路现有单 worker、
  无大小流双 QP 等限制。运行时 `srm_reroute_enable=1` 才启用自动检测；
  编译入路径池但运行时关闭检测，仍可用独立 CQ。

不支持独立 CQ 与关闭 CQE 简化组合；编译时报错，不静默回退。
运行时 `srm_reroute_enable=0` 不等于编译关闭换路。

独立 CQ 本身没有用户态开关，不修改 UAPI，也不要求用户库重编；但若改变
换路或 CQE 简化的公共宏，必须重编匹配的用户库。沿用现有构建方式：

```bash
# 仅在用户驱动的换路/CQE 简化宏改变时需要；确认应用加载的就是此 build 库。
cd /home/lingbo11/zxm/rdma-core
bash build.sh

# 先结束本次 RDMA 实验，再在维护窗口执行：原脚本会安装并卸载/重载模块。
cd /home/lingbo11/zxm/rdma-kerndriver
sudo bash kernel_make.sh
```

上述构建安装命令本次未执行。保留现有 kernel_make.sh，包括其
`KBUILD_NOCMDDEP=0`（宏变更必须触发重编），未增加另一套安装流程。
接收端的独立发送 CQ 是本地资源，不要求收发两端同时启用 PRIVATE_CQ；
双方原有共享 ABI/换路/CQE 简化配置仍应匹配。

恢复共享 CQ 只改内核 `MLX5_SRM_ENABLE_PRIVATE_CQ=0` 并重编安装；比较时
保持换路池与其他实验参数不变，否则物理 QP 数和执行路径也会变化。

## 实际变化

1. 每个物理 KQP 使用自己的发送 CQ；大小流打开时，两条物理 KQP 也各有 CQ。
   保留原来大小流与多 worker 的组合限制，不在此改动中扩展这些组合。
2. 新 poll 函数直接收到该 KQP 指针，不根据每个 CQE 的 QPN 查 radix tree，
   不维护去重链表。换路组空闲且 CQE 正常时，不逐 CQE 写 `sq.tail`、私有完成
   游标和共享 `cons_idx`；换路事务中则必须逐 CQE 走原有映射逻辑。
3. 仍检查 CQE owner/有效性、opcode、QPN、counter 和错误。正常连续 CQE
   只累计位置；counter 跳跃按实际跨度回收，不能把非连续完成简单当作 `+1`。
   但编译开启换路时首版所有用户 WQE 都为 signaled、每个 WQE 一个 BB；
   普通 CQE 不允许 counter 跳跃，只有回收 SQ 的维护 NOP 走专门处理。
   每次退出 poll 发布一次水位，空 CQ、短批次和到达预算都会退出，绝不等凑满。
   真正空 poll 不发布水位。错误记录和可选时间戳先于水位发布。
4. 单 CQ 待完成预算为该 KQP 的 `db_tail - cq_complete_idx`，在换路开启时使用
   63 位序号回绕比较；不再用整个 worker
   的 outstanding。用户 DB 也通过 db_tail 被发现。全局 issued/completed 信用
   仍保留，每个 poll 返回时只返还本次真实完成的 WQE 信用。
5. 每个 CQ 一次最多处理 POLL_BUDGET 个 CQE，仍有待完成则排回轮询环。
   信用耗尽时不困在一个 CQ 上忙等，继续发现其他 KQP 上的用户 DB。
6. CQ 初始按请求 SQ 深度创建，QP 创建后按真实 `2*wqe_cnt/3` 预留上限检查，
   不足则在建链前扩容。这样用户直接 DB 不会绕过 CQ 容量保证，亦不为每个 KQP
   固定复制原来全局 SQ_DEPTH 大小的 CQ。实际 CQ 深度由硬件的取整规则决定。
   活跃 SQ 窗口必须小于 65536，以确保低 16 位 counter 的扩展无歧义；过大时报错。
7. 释放流程覆盖全部独立 CQ；扩大后的代表表放堆上，不增加线程栈的大数组。
   同时将临时资源指针提前初始化为 NULL，避免分配失败清理未初始化指针。

`MLX5_SRM_CQE_PUBLISH_BATCH` 仍只控制共享 CQ 的合并路径。独立 CQ 使用
PRIVATE_CQ_POLL_BUDGET，并在 poll 返回时发布，两个参数不要混为一谈。
范围沿用历史 Hollow 的一 WR/一个 64B SQ basic block 模型，不是通用 verbs
可变长度 SQ WQE 解析器。普通 RC 的 CQ 路径不变。

首次独立 CQ 的生产代码改了四个文件：`scheduler.h`（配置与 CQ 表容量）、
`scheduler.c`（资源创建/释放、轮询和信用）、`cq.c`（专用 poll）、
`mlx5_ib.h`（函数声明）。适配换路的本次改动还涉及已有的 `reroute.inc`，
用于映射迁移完成及延迟发布逻辑水位；没有新增生产 `.inc` 文件。

## 换路开启时的完成路径

一个逻辑组通常有两条活动和两条备用物理 KQP；PRIVATE_CQ=1 给**每条物理
KQP**单独创建发送 CQ，不是每逻辑组一条 CQ。调度器 poll 每个 CQ 时分两路：

1. `RR_IDLE` 且 CQE 正常：每个 CQE 仅核验 QPN、opcode 和低 16 位 counter；
   在局部变量累加物理完成游标及真实信用。在 poll 退出时调用
   `mlx5_srm_rr_complete_idle_batch()`，更新该 KQP 的逻辑完成游标与可选
   拥塞检测字节数，然后只发布一次 `cons_idx`。自动检测关闭时不额外遍历
   WQE 描述符计算完成字节。
2. 冻结、复制、排空、回收或错误 CQE：逐条调用原有
   `mlx5_ib_srmc_complete_post()` 和 `mlx5_srm_rr_complete()`。目标 KQP
   上的迁移 CQE 会映射回源 KQP 的原始序号、错误与时间戳；只有映射确认的
   逻辑完成才能发布给用户。源 KQP 的回收 NOP 只返还一次硬件信用。
3. 每次 poll 消费 CQE 后先更新 CQ consumer index，再刷本轮涉及的逻辑
   `cons_idx`。源 KQP 在 active_path 指向目标后仍被轮询，直至原路和回收
   NOP 全部处理完；不会因为路径切换而遗失旧 CQE。

示意代码：

```c
if (idle_fast && opcode == MLX5_CQE_REQ) {
    /* 验证当前物理 SQ 的低 16 位 WQE counter */
    cursor = mlx5_srm_rr_seq(cursor + 1);
    fast_count++;
} else {
    /* 换路事务：逐 CQE 恢复源 KQP/原序号 */
    post = mlx5_ib_srmc_complete_post(qp, &qp->sq, counter, &credit_one);
    mlx5_srm_rr_complete(&origin, &post, status, vendor, tsc);
}
/* poll 返回：CQ CI 先于本轮逻辑水位发布 */
mlx5_cq_set_ci(&cq->mcq);
mlx5_srm_rr_complete_idle_batch(srmc, fast_start, fast_count);
mlx5_srm_rr_flush_native(sched);
```

生产代码实际对空 batch 和错误做条件判断；上面只是流程摘录。

## 确认生效

```bash
sudo dmesg | grep -E 'SRM CQ mode=|SRM private CQ ready'
```

模块初始化打印 `mode=private-per-kqp ... publish=poll-exit`；创建发送 KQP 时
打印 `kqp/qpn/cqn/cqe/sq_bbs/reserve_limit`。同 worker 的不同 KQP 应有不同 CQN。
启动日志是事件级，不增加逐 CQE 日志。旧日志可能来自此前装载，应检查时间戳。

## 验证及限制

```bash
cd /home/lingbo11/zxm/rdma-kerndriver
bash tests/run_private_cq_offline.sh
bash tests/run_reroute_offline.sh
```

独立 CQ 测试从生产文件提取函数，使用模拟 CQ，而非重写一份算法：
换路 0/1 × 预算 1/3/256 × WQE_TIMING 0/1，均使用 ASan/UBSan。
覆盖 64B/128B CQE、空/短批次、16/32/64 位游标回绕、逐 poll 只发布一次、
信用跨度、DB 快照边界、错误与后续 flush、错误 QPN/opcode/过期 counter、
resize 管理 CQE、设备错误和单 KQP 预算。换路离线组合增加 PRIVATE_CQ=1，
并覆盖空闲批量水位、迁移映射、源 CQ 排空和回收 NOP 信用。

对象编译检查复用现有 Kbuild `.cmd` 配方，输出到
`/tmp/srm-private-cq-compile-*`，不覆盖已有 root 所有的 `.o/.ko`。
这些不是可安装模块，也不是 RDMA 硬件验收。

最终对象验证目录：`/tmp/srm-private-cq-compile-final-f2vvn9z6`。检查
`cq/scheduler/qp/main` 四个翻译单元，以下五组均通过（共 20 次）：
`(PRIVATE,REROUTE,SIMPLIFY,BATCH,TIMING) = (1,0,1,1,0), (1,0,1,64,1),`
`(0,0,1,1,0), (0,0,0,1,0), (0,1,1,64,0)`；另确认两种不兼容配置被编译拒绝。
对象检查实际采用以下命令形式：

```bash
python3 - <<'PY'
import pathlib, shlex, subprocess, tempfile
repo = pathlib.Path('/home/lingbo11/zxm/rdma-kerndriver')
build = pathlib.Path('/lib/modules/5.4.0-86-generic/build').resolve()
out = pathlib.Path(tempfile.mkdtemp(prefix='srm-private-cq-compile-'))
print('Objects only:', out, flush=True)
for p,r,s,b,t in [(1,0,1,1,0), (1,0,1,64,1), (0,0,1,1,0),
                  (0,0,0,1,0), (0,1,1,64,0)]:
    name = f'p{p}-r{r}-s{s}-b{b}-t{t}'
    for source in ('cq', 'scheduler', 'qp', 'main'):
        recipe = repo / f'drivers/infiniband/hw/mlx5/.{source}.o.cmd'
        args = shlex.split(recipe.read_text().splitlines()[0].split(' := ',1)[1])
        assert args[0] == 'gcc' and '-c' in args
        args = [x for x in args if not x.startswith('-Wp,-MD,') and x != '-w']
        args[args.index('-o')+1] = str(out / f'{name}-{source}.o')
        args += [f'-DMLX5_SRM_ENABLE_PRIVATE_CQ={p}',
                 f'-DMLX5_SRM_ENABLE_REROUTE={r}',
                 f'-DMLX5_SRM_ENABLE_CQE_SIMPLIFY={s}',
                 f'-DMLX5_SRM_CQE_PUBLISH_BATCH={b}',
                 f'-DMLX5_SRM_ENABLE_WQE_TIMING={t}']
        result = subprocess.run(args, cwd=build, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.returncode:
            print(result.stdout)
            raise SystemExit(result.returncode)
        print('PASS', name, source, flush=True)
PY
```

硬件 A/B 还需在无其他实验运行的窗口执行：保持两组 REROUTE=1、SIMPLIFY=1、
相同 num_kqps、SQ 深度、limit_batch、CPU 和用户 DB 配置，只切 PRIVATE_CQ。
先短测 READ/WRITE 正确性及退出，再测吞吐；用户 DB 关/开分别对照。
CQ 更多会增加轮询固定成本，预算太小会增加轮转，太大会推迟 DB/其他 CQ；
并不承诺吞吐提升。若表现差，可试 64/256/1024 的预算，但应把预算变化单独记录。

本次换路适配另以 `-D` 覆盖宏（没有改工作树当前的 REROUTE=0），对
`(REROUTE,PRIVATE,SIMPLIFY,BATCH)=(1,1,1,1)、(1,1,1,64)、
(0,1,1,1)、(1,0,1,1)、(0,0,0,1)` 的 `cq/scheduler/qp/main`
对象编译均通过。换路＋独立 CQ＋WQE_TIMING=1 在 BATCH=1/64 的 8 个对象
编译也通过。两份离线测试全部通过；未安装/重载模块，也未完成真机换路吞吐验收。
