# 本地验证记录

## 2026-09-17：CQ/DB 常态路径优化 1–4

本轮在原目录的 `srm-reroute-size-aware` 修改，没有新建 worktree 或切换分支。
保留已有未提交的实验参数和构建产物。本轮没有 sudo、安装、模块重载、双机测试、
提交或推送；以下旧章节是初版实现时的历史记录，不代表当前工作区配置。

实际修改：`reroute.inc`、`scheduler.c`、离线测试、本文及设计文档。两个驱动的
`mlx5-srm-reroute.h` 仅同步字段用途注释，没有改变布局、能力或 ABI 值；rdma-core
执行代码没有修改，本轮不需要重新编译用户库（前提是双方原有宏已匹配）。

验证命令：

```bash
cd /home/lingbo11/zxm/rdma-kerndriver
bash tests/run_reroute_offline.sh
git diff --check -- drivers/infiniband/hw/mlx5/reroute.inc \
  drivers/infiniband/hw/mlx5/scheduler.c include/uapi/rdma/mlx5-srm-reroute.h \
  tests/reroute_state_test.c REROUTE_SIZE_AWARE.md REROUTE_VALIDATION.md
```

四种 ASan/UBSan 组合通过。新增检查普通完成不访问迁移表/位图/token、错误不走
快速路径、原路径/完整序号映射、完成私有计数不统计回收 NOP、共享保留字段不写、
不同 payload 的零/部分/完整信用字节累计，以及 16/48/63 位边界。

现有 `.o/.ko` 为 root 所有；为避免覆盖它们，编译检查复用 Kbuild 的 `.cmd`
参数，对生产 `scheduler.c` 和 `cq.c` 生成临时对象，不链接或安装。8 个组合：
`(R,S,B,T)=(0,0,1,0),(0,1,1,0),(1,0,1,0),(1,1,1,0),`
`(1,0,64,0),(1,1,64,0),(1,0,1,1),(1,1,64,1)`，共 16 次对象编译通过。
执行方式如下（只解析编译配方，不执行任意 shell 配方）：

```bash
python3 - <<'PY'
import pathlib, shlex, subprocess, tempfile
repo = pathlib.Path('/home/lingbo11/zxm/rdma-kerndriver')
build = pathlib.Path('/lib/modules/5.4.0-86-generic/build').resolve()
out = pathlib.Path(tempfile.mkdtemp(prefix='srm-reroute-cq-compile-'))
print('Compile objects only; artifacts:', out, flush=True)
configs = [(0,0,1,0), (0,1,1,0), (1,0,1,0), (1,1,1,0),
           (1,0,64,0), (1,1,64,0), (1,0,1,1), (1,1,64,1)]
for reroute, simplify, batch, timing in configs:
    name = f'r{reroute}-s{simplify}-b{batch}-t{timing}'
    for source in ('scheduler','cq'):
        recipe = repo / f'drivers/infiniband/hw/mlx5/.{source}.o.cmd'
        args = shlex.split(recipe.read_text().splitlines()[0].split(' := ',1)[1])
        assert args[0] == 'gcc' and '-c' in args
        args = [x for x in args if not x.startswith('-Wp,-MD,') and x != '-w']
        args[args.index('-o')+1] = str(out / f'{name}-{source}.o')
        args += [f'-DMLX5_SRM_ENABLE_REROUTE={reroute}',
                 f'-DMLX5_SRM_ENABLE_CQE_SIMPLIFY={simplify}',
                 f'-DMLX5_SRM_CQE_PUBLISH_BATCH={batch}',
                 f'-DMLX5_SRM_ENABLE_WQE_TIMING={timing}']
        result = subprocess.run(args, cwd=build, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.returncode:
            print(result.stdout)
            raise SystemExit(result.returncode)
        print('PASS', name, source, flush=True)
PY
```

上述是编译检查，不等于已生成可安装的新模块。正式构建安装仍使用原来的
`sudo bash kernel_make.sh`（会安装并卸载/重载模块，必须在停止实验的窗口执行）。
本轮尚未测量吞吐，也没有证明恢复到 180Gbps；性能比较要保持实际活动 KQP 数一致。

---

本记录对应 `srm-reroute-size-aware` 初版。结论仅限源码、编译和内存模拟，
**不是双机 RDMA 验收报告**。

## 隔离范围

- 两个独立 worktree 位于 `/home/lingbo11/zxm/reroute-size-aware/`。
- 用户驱动基线 `0f4cfea4`，内核基线 `a767575`。
- 原驱动目录的分支未切换；RDMA-General 的源码、脚本及实验参数未修改。
- 没有运行 `sudo`、安装命令、模块重装、SSH、远端同步或性能测试。
- 编译产生的已跟踪 `.cmd/.d/.mod/Module.symvers/modules.order` 文件恢复到
  worktree 的基线，只保留源码和文档修改，不提交生成物。
- 初次试编译发现基线跟踪的绝对路径生成物会触发原内核目录中 `.ko` 的重新
  链接，使用的是原目录已有对象文件；随后已修正构建路径和命令依赖检测。
  原目录由此次重新链接产生的 59 个 `.ko.cmd` 改动已恢复，源码和分支未动。
  原目录 `.ko` 的文件时间会改变；它们没有安装或加载。不要把旧构建产物当成
  本次换路实现的产物，正式安装前应在选定源码/宏配置下重新构建。

## 构建矩阵

`R`=REROUTE，`S`=CQE_SIMPLIFY，`T`=WQE_TIMING，`B`=CQE_PUBLISH_BATCH。
未列出的宏保持基线默认值。

| 组件 | 配置 | 结果 |
|---|---|---|
| mlx5 provider | R=0 | 编译通过 |
| mlx5 provider | R=1，S=1 | 编译通过 |
| mlx5 provider | R=1，S=0，T=1 | 编译通过 |
| mlx5 provider | R=1，S=1，T=1，DIRECT_USER_DB=0，TEST_HOOKS=1 | 编译通过 |
| 内核模块 | R=1，S=1 | 编译、链接通过 |
| 内核模块 | R=1，S=0，T=1 | 编译、链接通过 |
| 内核模块 | R=1，S=1，T=1，B=64 | 编译、链接通过 |
| 内核模块 | R=0 | 编译、链接通过；产物无 reroute 模块参数 |

用户驱动还执行过 `cmake --build build-reroute-on -j8` 的全项目构建。
最后一次源码检查后又重建了上述四种配置的 mlx5 provider。
内核工作区最后生成的是 **R=0** 的模块，不要误以为当前 `.ko` 开启了换路。

实际命令形式如下；每种配置依次执行，没有并发编译同一内核目录：

```bash
# 在 rdma-core worktree；其他配置使用不同 -B 目录和对应 CFLAGS
cmake -S . -B build-reroute-on \
  -DIN_PLACE=1 -DCMAKE_BUILD_TYPE=Release \
  -DNO_MAN_PAGES=1 -DENABLE_PYVERBS=0 \
  '-DCMAKE_C_FLAGS=-DMLX5_SRM_ENABLE_REROUTE=1'
cmake --build build-reroute-on --target mlx5 -j8

# 在内核 worktree；KCFLAGS 分别替换成上表各配置
reroute_kernel_dir=$(pwd -P)
make -j8 kernel \
  CWD="$reroute_kernel_dir" \
  AUTOCONF_H="$reroute_kernel_dir/include/generated/autoconf.h" \
  WITH_MAKE_PARAMS=KBUILD_NOCMDDEP=0 \
  'KCFLAGS=-DMLX5_SRM_ENABLE_REROUTE=1'
```

`KBUILD_NOCMDDEP=0` 必须保留，否则该仓库原构建选项会忽略宏变化，可能混用
不同 CQE 模式的对象文件。本次没有修改原 makefile。
完整本机内核日志保存在相邻 `verification-logs/kernel/`；provider 最终日志
在各 `build-reroute-*/build-final.log`，日志和二进制不纳入分支提交。

## 离线测试

```bash
bash tests/run_reroute_offline.sh
```

四种组合 `S=0/1 × B=1/64`，均通过 ASan/UBSan。每种组合直接编译生产状态机，
覆盖 240 次按总量分类的迁移和 4800 次空区间复用；包含：

- 10239/10240/10241B、多 WQE 总和、空区间；
- token 缺槽持续保持同一冻结事务；owner 忙时退出本次访问；
- 64 项检查/复制预算、稳定依赖分组、目标预写/DB 门禁；
- 完成还原与前缀位图、传输错误后关闭门禁并保留映射；
- 严格连续检测、关闭检测不取消已开始的事务；
- 迁出源槽 NOP 回收、一个维护信用、收到维护 CQE 前不得复用；
- 16 位 CQE、48 位 token、63 位冻结游标的回绕边界；
- 两仓库 ABI 头文件逐字节一致。

模拟 CQE 的到达由测试驱动，不能证明真实设备的 DMA 可见性、NOP 消费、
网络顺序、应用 WR ID 全链路、真实并发销毁或资源泄漏。编译用户 DB 开关不等于
已经执行了用户 DB 开/关的硬件测试矩阵。

## 待确认窗口后执行

详见 `REROUTE_SIZE_AWARE.md` 的硬件验收矩阵。至少完成 READ/WRITE 数据校验、
简化开关 × 用户 DB 开关、延迟 token、交错完成、退出清理和连续 20 轮测试，
才可判断是否能替换当前实验版本。吞吐提升没有预先保证。
