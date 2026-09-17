# 本地验证记录

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
