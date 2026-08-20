cmd_/home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_accel/ktls_txrx.o := gcc -Wp,-MD,/home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_accel/.ktls_txrx.o.d  -nostdinc -isystem /usr/lib/gcc/x86_64-linux-gnu/9/include -D__OFED_BUILD__ -D__KERNEL__ -DCOMPAT_BASE="\"mlnx-ofa_kernel-compat-20241202-1723-77239cf\"" -DCOMPAT_BASE_TREE="\"https://:@git-nbu.nvidia.com/r/a/mlnx_ofed/mlnx-ofa_kernel-4.0.git\"" -DCOMPAT_BASE_TREE_VERSION="\"77239cf\"" -DCOMPAT_PROJECT="\"Compat-mlnx-ofed\"" -DCOMPAT_VERSION="\"77239cf\""  -include /lib/modules/5.4.0-86-generic/build/include/generated/autoconf.h -include /lib/modules/5.4.0-86-generic/build/include/linux/kconfig.h -include /home/lingbo11/zxm/rdma-kerndriver/include/linux/compat-2.6.h  -include /home/lingbo11/zxm/rdma-kerndriver/drivers/infiniband/debug/mtrack.h  -I/home/lingbo11/zxm/rdma-kerndriver/include -I/home/lingbo11/zxm/rdma-kerndriver/include/uapi -I/home/lingbo11/zxm/rdma-kerndriver/drivers/infiniband/debug -D__XEN_INTERFACE_VERSION__= -I./arch/x86/include/mach-xen -I./arch/x86/include -Iarch/x86/include/generated -Iinclude -I./arch/x86/include/uapi -Iarch/x86/include/generated/uapi -I./include -I./include/uapi -Iinclude/generated/uapi  -I./arch/x86/include -Iarch/x86/include/generated  -include ./include/linux/compiler_types.h -D__KERNEL__ -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Wno-format-security -std=gnu89 -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -fcf-protection=none -m64 -falign-jumps=1 -falign-loops=1 -mno-80387 -mno-fp-ret-in-387 -mpreferred-stack-boundary=3 -mskip-rax-setup -mtune=generic -mno-red-zone -mcmodel=kernel -DCONFIG_X86_X32_ABI -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 -DCONFIG_AS_CFI_SECTIONS=1 -DCONFIG_AS_SSSE3=1 -DCONFIG_AS_AVX=1 -DCONFIG_AS_AVX2=1 -DCONFIG_AS_AVX512=1 -DCONFIG_AS_SHA1_NI=1 -DCONFIG_AS_SHA256_NI=1 -Wno-sign-compare -fno-asynchronous-unwind-tables -mindirect-branch=thunk-extern -mindirect-branch-register -fno-jump-tables -fno-delete-null-pointer-checks -Wno-frame-address -Wno-format-truncation -Wno-format-overflow -Wno-address-of-packed-member -O2 --param=allow-store-data-races=0 -Wframe-larger-than=1024 -fstack-protector-strong -Wimplicit-fallthrough -Wno-unused-but-set-variable -Wno-unused-const-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-var-tracking-assignments -g -pg -mrecord-mcount -mfentry -DCC_USING_FENTRY -flive-patching=inline-clone -Wdeclaration-after-statement -Wvla -Wno-pointer-sign -Wno-stringop-truncation -Wno-array-bounds -Wno-stringop-overflow -Wno-restrict -Wno-maybe-uninitialized -fno-strict-overflow -fno-merge-all-constants -fmerge-constants -fno-stack-check -fconserve-stack -Werror=date-time -Werror=incompatible-pointer-types -Werror=designated-init -fmacro-prefix-map=./= -Wno-packed-not-aligned -Wno-date-time -I/home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core  -DMODULE  -DKBUILD_BASENAME='"ktls_txrx"' -DKBUILD_MODNAME='"mlx5_core"' -c -o /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_accel/ktls_txrx.o /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_accel/ktls_txrx.c

source_/home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_accel/ktls_txrx.o := /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_accel/ktls_txrx.c

deps_/home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_accel/ktls_txrx.o := \
  /lib/modules/5.4.0-86-generic/build/include/linux/kconfig.h \
    $(wildcard include/config/cpu/big/endian.h) \
    $(wildcard include/config/booger.h) \
    $(wildcard include/config/foo.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/compat-2.6.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/../../compat/config.h \
  include/generated/uapi/linux/version.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/kconfig.h \
  include/linux/kconfig.h \
  include/uapi/linux/if.h \
  include/uapi/linux/libc-compat.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/types.h \
  include/linux/types.h \
    $(wildcard include/config/have/uid16.h) \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  include/uapi/linux/types.h \
  arch/x86/include/generated/uapi/asm/types.h \
  include/uapi/asm-generic/types.h \
  include/asm-generic/int-ll64.h \
  include/uapi/asm-generic/int-ll64.h \
  arch/x86/include/uapi/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/uapi/asm-generic/bitsperlong.h \
  include/uapi/linux/posix_types.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/stddef.h \
  include/linux/stddef.h \
  include/uapi/linux/stddef.h \
  include/linux/compiler_types.h \
    $(wildcard include/config/have/arch/compiler/h.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/optimize/inlining.h) \
    $(wildcard include/config/cc/has/asm/inline.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/compiler_attributes.h \
  include/linux/compiler_attributes.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/compiler-gcc.h \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arm64.h) \
    $(wildcard include/config/retpoline.h) \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  arch/x86/include/asm/posix_types.h \
    $(wildcard include/config/x86/32.h) \
  arch/x86/include/uapi/asm/posix_types_64.h \
  include/uapi/asm-generic/posix_types.h \
  include/linux/socket.h \
    $(wildcard include/config/proc/fs.h) \
    $(wildcard include/config/compat.h) \
  arch/x86/include/generated/uapi/asm/socket.h \
  include/uapi/asm-generic/socket.h \
  arch/x86/include/generated/uapi/asm/sockios.h \
  include/uapi/asm-generic/sockios.h \
  include/uapi/linux/sockios.h \
  include/linux/uio.h \
    $(wildcard include/config/arch/has/uaccess/flushcache.h) \
    $(wildcard include/config/arch/has/uaccess/mcsafe.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/kernel.h \
  include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/arch/has/refcount.h) \
    $(wildcard include/config/panic/timeout.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /usr/lib/gcc/x86_64-linux-gnu/9/include/stdarg.h \
  include/linux/limits.h \
  include/uapi/linux/limits.h \
  include/linux/linkage.h \
  include/linux/stringify.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/export.h \
  include/linux/export.h \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/module/rel/crcs.h) \
    $(wildcard include/config/have/arch/prel32/relocations.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/trim/unused/ksyms.h) \
    $(wildcard include/config/unused/symbols.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/compiler.h \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/stack/validation.h) \
    $(wildcard include/config/debug/entry.h) \
    $(wildcard include/config/kasan.h) \
  arch/x86/include/asm/barrier.h \
  arch/x86/include/asm/alternative.h \
    $(wildcard include/config/smp.h) \
  arch/x86/include/asm/asm.h \
  arch/x86/include/asm/nops.h \
    $(wildcard include/config/mk7.h) \
    $(wildcard include/config/x86/p6/nop.h) \
    $(wildcard include/config/x86/64.h) \
  include/asm-generic/barrier.h \
  include/linux/kasan-checks.h \
  arch/x86/include/asm/linkage.h \
    $(wildcard include/config/x86/alignment/16.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/bitops.h \
  include/linux/bitops.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/bits.h \
  include/linux/bits.h \
  include/linux/const.h \
  include/uapi/linux/const.h \
  arch/x86/include/asm/bitops.h \
    $(wildcard include/config/x86/cmov.h) \
  arch/x86/include/asm/rmwcc.h \
    $(wildcard include/config/cc/has/asm/goto.h) \
  include/asm-generic/bitops/find.h \
    $(wildcard include/config/generic/find/first/bit.h) \
  include/asm-generic/bitops/sched.h \
  arch/x86/include/asm/arch_hweight.h \
  arch/x86/include/asm/cpufeatures.h \
  arch/x86/include/asm/required-features.h \
    $(wildcard include/config/x86/minimum/cpu/family.h) \
    $(wildcard include/config/math/emulation.h) \
    $(wildcard include/config/x86/pae.h) \
    $(wildcard include/config/x86/cmpxchg64.h) \
    $(wildcard include/config/x86/use/3dnow.h) \
    $(wildcard include/config/matom.h) \
    $(wildcard include/config/paravirt.h) \
  arch/x86/include/asm/disabled-features.h \
    $(wildcard include/config/x86/intel/mpx.h) \
    $(wildcard include/config/x86/smap.h) \
    $(wildcard include/config/x86/intel/umip.h) \
    $(wildcard include/config/x86/intel/memory/protection/keys.h) \
    $(wildcard include/config/x86/5level.h) \
    $(wildcard include/config/page/table/isolation.h) \
  include/asm-generic/bitops/const_hweight.h \
  include/asm-generic/bitops-instrumented.h \
  include/asm-generic/bitops/le.h \
  arch/x86/include/uapi/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/uapi/linux/byteorder/little_endian.h \
  include/linux/swab.h \
  include/uapi/linux/swab.h \
  arch/x86/include/uapi/asm/swab.h \
  include/linux/byteorder/generic.h \
  include/asm-generic/bitops/ext2-atomic-setbit.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/log2.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/linux/typecheck.h \
  include/linux/printk.h \
    $(wildcard include/config/message/loglevel/default.h) \
    $(wildcard include/config/console/loglevel/default.h) \
    $(wildcard include/config/console/loglevel/quiet.h) \
    $(wildcard include/config/early/printk.h) \
    $(wildcard include/config/printk/nmi.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/kmsg/ids.h) \
    $(wildcard include/config/dynamic/debug.h) \
  include/linux/init.h \
    $(wildcard include/config/strict/kernel/rwx.h) \
    $(wildcard include/config/strict/module/rwx.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/kern_levels.h \
  include/linux/kern_levels.h \
  include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  include/uapi/linux/kernel.h \
  include/uapi/linux/sysinfo.h \
  arch/x86/include/asm/cache.h \
    $(wildcard include/config/x86/l1/cache/shift.h) \
    $(wildcard include/config/x86/internode/cache/shift.h) \
    $(wildcard include/config/x86/vsmp.h) \
  include/linux/dynamic_debug.h \
    $(wildcard include/config/jump/label.h) \
  include/linux/jump_label.h \
    $(wildcard include/config/have/arch/jump/label/relative.h) \
  arch/x86/include/asm/jump_label.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/build_bug.h \
  include/linux/build_bug.h \
  arch/x86/include/asm/div64.h \
  include/asm-generic/div64.h \
  include/linux/thread_info.h \
    $(wildcard include/config/thread/info/in/task.h) \
    $(wildcard include/config/have/arch/within/stack/frames.h) \
    $(wildcard include/config/hardened/usercopy.h) \
  include/linux/bug.h \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/bug/on/data/corruption.h) \
  arch/x86/include/asm/bug.h \
    $(wildcard include/config/debug/bugverbose.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/asm-generic/bug.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/asm-generic/../../compat/config.h \
  include/asm-generic/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/generic/bug/relative/pointers.h) \
  include/linux/restart_block.h \
  include/linux/time64.h \
  include/linux/math64.h \
    $(wildcard include/config/arch/supports/int128.h) \
  include/uapi/linux/time.h \
  include/uapi/linux/time_types.h \
  include/linux/errno.h \
  include/uapi/linux/errno.h \
  arch/x86/include/generated/uapi/asm/errno.h \
  include/uapi/asm-generic/errno.h \
  include/uapi/asm-generic/errno-base.h \
  arch/x86/include/asm/current.h \
  arch/x86/include/asm/percpu.h \
    $(wildcard include/config/x86/64/smp.h) \
  include/asm-generic/percpu.h \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/have/setup/per/cpu/area.h) \
  include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  include/linux/percpu-defs.h \
    $(wildcard include/config/debug/force/weak/per/cpu.h) \
    $(wildcard include/config/amd/mem/encrypt.h) \
  arch/x86/include/asm/thread_info.h \
    $(wildcard include/config/vm86.h) \
    $(wildcard include/config/frame/pointer.h) \
    $(wildcard include/config/ia32/emulation.h) \
  arch/x86/include/asm/page.h \
  arch/x86/include/asm/page_types.h \
    $(wildcard include/config/physical/start.h) \
    $(wildcard include/config/physical/align.h) \
    $(wildcard include/config/dynamic/physical/mask.h) \
  include/linux/mem_encrypt.h \
    $(wildcard include/config/arch/has/mem/encrypt.h) \
  arch/x86/include/asm/mem_encrypt.h \
  arch/x86/include/uapi/asm/bootparam.h \
  include/linux/screen_info.h \
  include/uapi/linux/screen_info.h \
  include/linux/apm_bios.h \
  include/uapi/linux/apm_bios.h \
  include/uapi/linux/ioctl.h \
  arch/x86/include/generated/uapi/asm/ioctl.h \
  include/asm-generic/ioctl.h \
  include/uapi/asm-generic/ioctl.h \
  include/linux/edd.h \
  include/uapi/linux/edd.h \
  arch/x86/include/asm/ist.h \
  arch/x86/include/uapi/asm/ist.h \
  include/video/edid.h \
    $(wildcard include/config/x86.h) \
  include/uapi/video/edid.h \
  arch/x86/include/asm/page_64_types.h \
    $(wildcard include/config/dynamic/memory/layout.h) \
    $(wildcard include/config/randomize/base.h) \
  arch/x86/include/asm/kaslr.h \
    $(wildcard include/config/randomize/memory.h) \
  arch/x86/include/asm/page_64.h \
    $(wildcard include/config/debug/virtual.h) \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/x86/vsyscall/emulation.h) \
  include/linux/range.h \
  include/asm-generic/memory_model.h \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/sparsemem.h) \
  include/linux/pfn.h \
  include/asm-generic/getorder.h \
  arch/x86/include/asm/cpufeature.h \
    $(wildcard include/config/x86/feature/names.h) \
  arch/x86/include/asm/processor.h \
    $(wildcard include/config/kvm.h) \
    $(wildcard include/config/stackprotector.h) \
    $(wildcard include/config/paravirt/xxl.h) \
    $(wildcard include/config/x86/debugctlmsr.h) \
    $(wildcard include/config/cpu/sup/amd.h) \
    $(wildcard include/config/xen.h) \
  arch/x86/include/asm/processor-flags.h \
  arch/x86/include/uapi/asm/processor-flags.h \
  arch/x86/include/asm/math_emu.h \
  arch/x86/include/asm/ptrace.h \
  arch/x86/include/asm/segment.h \
    $(wildcard include/config/xen/pv.h) \
    $(wildcard include/config/x86/32/lazy/gs.h) \
  arch/x86/include/uapi/asm/ptrace.h \
  arch/x86/include/uapi/asm/ptrace-abi.h \
  arch/x86/include/asm/paravirt_types.h \
    $(wildcard include/config/pgtable/levels.h) \
    $(wildcard include/config/paravirt/debug.h) \
  arch/x86/include/asm/desc_defs.h \
  arch/x86/include/asm/kmap_types.h \
    $(wildcard include/config/debug/highmem.h) \
  include/asm-generic/kmap_types.h \
  arch/x86/include/asm/pgtable_types.h \
    $(wildcard include/config/mem/soft/dirty.h) \
  arch/x86/include/asm/pgtable_64_types.h \
  arch/x86/include/asm/sparsemem.h \
  include/asm-generic/pgtable-nop4d.h \
  arch/x86/include/asm/nospec-branch.h \
  include/linux/static_key.h \
  arch/x86/include/asm/alternative-asm.h \
  arch/x86/include/asm/msr-index.h \
  arch/x86/include/asm/spinlock_types.h \
    $(wildcard include/config/paravirt/spinlocks.h) \
  include/asm-generic/qspinlock_types.h \
  include/asm-generic/qrwlock_types.h \
  arch/x86/include/uapi/asm/sigcontext.h \
  arch/x86/include/asm/msr.h \
    $(wildcard include/config/tracepoints.h) \
  arch/x86/include/asm/msr-index.h \
  arch/x86/include/asm/cpumask.h \
  include/linux/cpumask.h \
    $(wildcard include/config/cpumask/offstack.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/debug/per/cpu/maps.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/bitmap.h \
  include/linux/bitmap.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/string.h \
  include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
    $(wildcard include/config/fortify/source.h) \
  include/uapi/linux/string.h \
  arch/x86/include/asm/string.h \
  arch/x86/include/asm/string_64.h \
    $(wildcard include/config/x86/mce.h) \
  include/linux/atomic.h \
  arch/x86/include/asm/atomic.h \
  arch/x86/include/asm/cmpxchg.h \
  arch/x86/include/asm/cmpxchg_64.h \
  arch/x86/include/asm/atomic64_64.h \
  include/asm-generic/atomic-instrumented.h \
  include/linux/atomic-fallback.h \
    $(wildcard include/config/generic/atomic64.h) \
  include/asm-generic/atomic-long.h \
  arch/x86/include/uapi/asm/msr.h \
  include/linux/tracepoint-defs.h \
  arch/x86/include/asm/paravirt.h \
  arch/x86/include/asm/frame.h \
  arch/x86/include/asm/special_insns.h \
  arch/x86/include/asm/fpu/types.h \
  arch/x86/include/asm/unwind_hints.h \
  arch/x86/include/asm/orc_types.h \
  include/linux/personality.h \
  include/uapi/linux/personality.h \
  include/linux/err.h \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  arch/x86/include/asm/irqflags.h \
    $(wildcard include/config/debug/lock/alloc.h) \
  include/crypto/hash.h \
  include/linux/crypto.h \
    $(wildcard include/config/crypto/stats.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/list.h \
  include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
    $(wildcard include/config/page/poisoning/zero.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/slab.h \
  include/linux/slab.h \
    $(wildcard include/config/debug/slab.h) \
    $(wildcard include/config/debug/objects.h) \
    $(wildcard include/config/failslab.h) \
    $(wildcard include/config/memcg/kmem.h) \
    $(wildcard include/config/have/hardened/usercopy/allocator.h) \
    $(wildcard include/config/slab.h) \
    $(wildcard include/config/slub.h) \
    $(wildcard include/config/slob.h) \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/numa.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/gfp.h \
  include/linux/gfp.h \
    $(wildcard include/config/lockdep.h) \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/zone/dma32.h) \
    $(wildcard include/config/zone/device.h) \
    $(wildcard include/config/pm/sleep.h) \
    $(wildcard include/config/contig/alloc.h) \
    $(wildcard include/config/cma.h) \
  include/linux/mmdebug.h \
    $(wildcard include/config/debug/vm.h) \
    $(wildcard include/config/debug/vm/pgflags.h) \
  include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
    $(wildcard include/config/memory/isolation.h) \
    $(wildcard include/config/shuffle/page/allocator.h) \
    $(wildcard include/config/zsmalloc.h) \
    $(wildcard include/config/memcg.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/compaction.h) \
    $(wildcard include/config/transparent/hugepage.h) \
    $(wildcard include/config/flat/node/mem/map.h) \
    $(wildcard include/config/page/extension.h) \
    $(wildcard include/config/deferred/struct/page/init.h) \
    $(wildcard include/config/have/memory/present.h) \
    $(wildcard include/config/have/memoryless/nodes.h) \
    $(wildcard include/config/have/memblock/node/map.h) \
    $(wildcard include/config/need/multiple/nodes.h) \
    $(wildcard include/config/have/arch/early/pfn/to/nid.h) \
    $(wildcard include/config/sparsemem/extreme.h) \
    $(wildcard include/config/memory/hotremove.h) \
    $(wildcard include/config/have/arch/pfn/valid.h) \
    $(wildcard include/config/holes/in/zone.h) \
    $(wildcard include/config/arch/has/holes/memorymodel.h) \
  include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/preemption.h) \
  include/linux/preempt.h \
    $(wildcard include/config/preempt/count.h) \
    $(wildcard include/config/trace/preempt/toggle.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  arch/x86/include/asm/preempt.h \
  include/linux/bottom_half.h \
  arch/x86/include/generated/asm/mmiowb.h \
  include/asm-generic/mmiowb.h \
    $(wildcard include/config/mmiowb.h) \
  include/linux/spinlock_types.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/lockdep.h \
  include/linux/lockdep.h \
    $(wildcard include/config/lock/stat.h) \
  include/linux/rwlock_types.h \
  arch/x86/include/asm/spinlock.h \
  arch/x86/include/asm/qspinlock.h \
  include/asm-generic/qspinlock.h \
  arch/x86/include/asm/qrwlock.h \
  include/asm-generic/qrwlock.h \
  include/linux/rwlock.h \
    $(wildcard include/config/preempt.h) \
  include/linux/spinlock_api_smp.h \
    $(wildcard include/config/inline/spin/lock.h) \
    $(wildcard include/config/inline/spin/lock/bh.h) \
    $(wildcard include/config/inline/spin/lock/irq.h) \
    $(wildcard include/config/inline/spin/lock/irqsave.h) \
    $(wildcard include/config/inline/spin/trylock.h) \
    $(wildcard include/config/inline/spin/trylock/bh.h) \
    $(wildcard include/config/uninline/spin/unlock.h) \
    $(wildcard include/config/inline/spin/unlock/bh.h) \
    $(wildcard include/config/inline/spin/unlock/irq.h) \
    $(wildcard include/config/inline/spin/unlock/irqrestore.h) \
    $(wildcard include/config/generic/lockbreak.h) \
  include/linux/rwlock_api_smp.h \
    $(wildcard include/config/inline/read/lock.h) \
    $(wildcard include/config/inline/write/lock.h) \
    $(wildcard include/config/inline/read/lock/bh.h) \
    $(wildcard include/config/inline/write/lock/bh.h) \
    $(wildcard include/config/inline/read/lock/irq.h) \
    $(wildcard include/config/inline/write/lock/irq.h) \
    $(wildcard include/config/inline/read/lock/irqsave.h) \
    $(wildcard include/config/inline/write/lock/irqsave.h) \
    $(wildcard include/config/inline/read/trylock.h) \
    $(wildcard include/config/inline/write/trylock.h) \
    $(wildcard include/config/inline/read/unlock.h) \
    $(wildcard include/config/inline/write/unlock.h) \
    $(wildcard include/config/inline/read/unlock/bh.h) \
    $(wildcard include/config/inline/write/unlock/bh.h) \
    $(wildcard include/config/inline/read/unlock/irq.h) \
    $(wildcard include/config/inline/write/unlock/irq.h) \
    $(wildcard include/config/inline/read/unlock/irqrestore.h) \
    $(wildcard include/config/inline/write/unlock/irqrestore.h) \
  include/linux/wait.h \
  include/uapi/linux/wait.h \
  include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
  include/linux/seqlock.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/nodemask.h \
  include/linux/nodemask.h \
  include/linux/pageblock-flags.h \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/hugetlb/page/size/variable.h) \
  include/linux/page-flags-layout.h \
    $(wildcard include/config/numa/balancing.h) \
    $(wildcard include/config/kasan/sw/tags.h) \
  include/generated/bounds.h \
  include/linux/mm_types.h \
    $(wildcard include/config/have/aligned/struct/page.h) \
    $(wildcard include/config/userfaultfd.h) \
    $(wildcard include/config/swap.h) \
    $(wildcard include/config/have/arch/compat/mmap/bases.h) \
    $(wildcard include/config/membarrier.h) \
    $(wildcard include/config/aio.h) \
    $(wildcard include/config/mmu/notifier.h) \
    $(wildcard include/config/arch/want/batched/unmap/tlb/flush.h) \
  include/linux/mm_types_task.h \
    $(wildcard include/config/split/ptlock/cpus.h) \
    $(wildcard include/config/arch/enable/split/pmd/ptlock.h) \
  arch/x86/include/asm/tlbbatch.h \
  include/linux/auxvec.h \
  include/uapi/linux/auxvec.h \
  arch/x86/include/uapi/asm/auxvec.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/rbtree.h \
  include/linux/rbtree.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/rcupdate.h \
  include/linux/rcupdate.h \
    $(wildcard include/config/preempt/rcu.h) \
    $(wildcard include/config/rcu/stall/common.h) \
    $(wildcard include/config/no/hz/full.h) \
    $(wildcard include/config/rcu/nocb/cpu.h) \
    $(wildcard include/config/tasks/rcu.h) \
    $(wildcard include/config/tree/rcu.h) \
    $(wildcard include/config/tiny/rcu.h) \
    $(wildcard include/config/debug/objects/rcu/head.h) \
    $(wildcard include/config/prove/rcu.h) \
    $(wildcard include/config/rcu/boost.h) \
    $(wildcard include/config/arch/weak/release/acquire.h) \
  include/linux/rcutree.h \
  include/linux/rwsem.h \
    $(wildcard include/config/rwsem/spin/on/owner.h) \
    $(wildcard include/config/debug/rwsems.h) \
  include/linux/osq_lock.h \
  include/linux/completion.h \
  include/linux/uprobes.h \
    $(wildcard include/config/uprobes.h) \
  arch/x86/include/asm/uprobes.h \
  include/linux/notifier.h \
    $(wildcard include/config/tree/srcu.h) \
  include/linux/mutex.h \
    $(wildcard include/config/mutex/spin/on/owner.h) \
    $(wildcard include/config/debug/mutexes.h) \
  include/linux/debug_locks.h \
    $(wildcard include/config/debug/locking/api/selftests.h) \
  include/linux/srcu.h \
    $(wildcard include/config/tiny/srcu.h) \
    $(wildcard include/config/srcu.h) \
  include/linux/workqueue.h \
    $(wildcard include/config/debug/objects/work.h) \
    $(wildcard include/config/freezer.h) \
    $(wildcard include/config/sysfs.h) \
    $(wildcard include/config/wq/watchdog.h) \
  include/linux/timer.h \
    $(wildcard include/config/debug/objects/timers.h) \
    $(wildcard include/config/preempt/rt.h) \
    $(wildcard include/config/no/hz/common.h) \
  include/linux/ktime.h \
  include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
  include/linux/time32.h \
  include/linux/timex.h \
  include/uapi/linux/timex.h \
  include/uapi/linux/param.h \
  arch/x86/include/generated/uapi/asm/param.h \
  include/asm-generic/param.h \
    $(wildcard include/config/hz.h) \
  include/uapi/asm-generic/param.h \
  arch/x86/include/asm/timex.h \
  arch/x86/include/asm/tsc.h \
    $(wildcard include/config/x86/tsc.h) \
  include/linux/jiffies.h \
  include/generated/timeconst.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/timekeeping.h \
  include/linux/timekeeping.h \
  include/linux/timekeeping32.h \
  include/linux/debugobjects.h \
    $(wildcard include/config/debug/objects/free.h) \
  include/linux/rcu_segcblist.h \
  include/linux/srcutree.h \
  include/linux/rcu_node_tree.h \
    $(wildcard include/config/rcu/fanout.h) \
    $(wildcard include/config/rcu/fanout/leaf.h) \
  arch/x86/include/asm/mmu.h \
    $(wildcard include/config/modify/ldt/syscall.h) \
  include/linux/page-flags.h \
    $(wildcard include/config/arch/uses/pg/uncached.h) \
    $(wildcard include/config/memory/failure.h) \
    $(wildcard include/config/idle/page/tracking.h) \
    $(wildcard include/config/thp/swap.h) \
    $(wildcard include/config/ksm.h) \
  include/linux/memory_hotplug.h \
    $(wildcard include/config/arch/has/add/pages.h) \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
    $(wildcard include/config/have/bootmem/info/node.h) \
  arch/x86/include/asm/mmzone.h \
  arch/x86/include/asm/mmzone_64.h \
  arch/x86/include/asm/smp.h \
    $(wildcard include/config/x86/local/apic.h) \
    $(wildcard include/config/x86/io/apic.h) \
    $(wildcard include/config/debug/nmi/selftest.h) \
  arch/x86/include/asm/mpspec.h \
    $(wildcard include/config/eisa.h) \
    $(wildcard include/config/x86/mpparse.h) \
  arch/x86/include/asm/mpspec_def.h \
  arch/x86/include/asm/x86_init.h \
  arch/x86/include/asm/apicdef.h \
  arch/x86/include/asm/apic.h \
    $(wildcard include/config/x86/x2apic.h) \
  arch/x86/include/asm/fixmap.h \
    $(wildcard include/config/provide/ohci1394/dma/init.h) \
    $(wildcard include/config/pci/mmconfig.h) \
    $(wildcard include/config/x86/intel/mid.h) \
    $(wildcard include/config/acpi/apei/ghes.h) \
    $(wildcard include/config/intel/txt.h) \
  arch/x86/include/asm/acpi.h \
    $(wildcard include/config/acpi/apei.h) \
    $(wildcard include/config/acpi.h) \
    $(wildcard include/config/acpi/numa.h) \
  include/acpi/pdc_intel.h \
  arch/x86/include/asm/numa.h \
    $(wildcard include/config/numa/emu.h) \
  arch/x86/include/asm/topology.h \
    $(wildcard include/config/sched/mc/prio.h) \
  include/asm-generic/topology.h \
  arch/x86/include/asm/realmode.h \
    $(wildcard include/config/acpi/sleep.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/asm/io.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/asm/../../compat/config.h \
  arch/x86/include/asm/io.h \
    $(wildcard include/config/mtrr.h) \
    $(wildcard include/config/x86/pat.h) \
  arch/x86/include/generated/asm/early_ioremap.h \
  include/asm-generic/early_ioremap.h \
    $(wildcard include/config/generic/early/ioremap.h) \
  include/asm-generic/iomap.h \
    $(wildcard include/config/has/ioport/map.h) \
    $(wildcard include/config/pci.h) \
    $(wildcard include/config/generic/iomap.h) \
  include/asm-generic/pci_iomap.h \
    $(wildcard include/config/no/generic/pci/ioport/map.h) \
    $(wildcard include/config/generic/pci/iomap.h) \
  include/asm-generic/io.h \
    $(wildcard include/config/virt/to/bus.h) \
  include/linux/logic_pio.h \
    $(wildcard include/config/indirect/pio.h) \
  include/linux/fwnode.h \
  include/linux/vmalloc.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/llist.h \
  include/linux/llist.h \
    $(wildcard include/config/arch/have/nmi/safe/cmpxchg.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/overflow.h \
  include/linux/overflow.h \
  arch/x86/include/uapi/asm/vsyscall.h \
  include/asm-generic/fixmap.h \
  arch/x86/include/asm/hardirq.h \
    $(wildcard include/config/kvm/intel.h) \
    $(wildcard include/config/have/kvm.h) \
    $(wildcard include/config/x86/thermal/vector.h) \
    $(wildcard include/config/x86/mce/threshold.h) \
    $(wildcard include/config/x86/mce/amd.h) \
    $(wildcard include/config/x86/hv/callback/vector.h) \
    $(wildcard include/config/hyperv.h) \
  arch/x86/include/asm/io_apic.h \
  arch/x86/include/asm/irq_vectors.h \
    $(wildcard include/config/pci/msi.h) \
  include/linux/topology.h \
    $(wildcard include/config/use/percpu/numa/node/id.h) \
    $(wildcard include/config/sched/smt.h) \
  include/linux/arch_topology.h \
    $(wildcard include/config/generic/arch/topology.h) \
  include/linux/percpu.h \
    $(wildcard include/config/need/per/cpu/embed/first/chunk.h) \
    $(wildcard include/config/need/per/cpu/page/first/chunk.h) \
  include/linux/smp.h \
    $(wildcard include/config/up/late/init.h) \
  include/linux/percpu-refcount.h \
  include/linux/kasan.h \
    $(wildcard include/config/kasan/generic.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/uaccess.h \
  include/linux/uaccess.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/sched.h \
  include/linux/sched.h \
    $(wildcard include/config/virt/cpu/accounting/native.h) \
    $(wildcard include/config/sched/info.h) \
    $(wildcard include/config/schedstats.h) \
    $(wildcard include/config/fair/group/sched.h) \
    $(wildcard include/config/rt/group/sched.h) \
    $(wildcard include/config/uclamp/task.h) \
    $(wildcard include/config/uclamp/buckets/count.h) \
    $(wildcard include/config/cgroup/sched.h) \
    $(wildcard include/config/blk/dev/io/trace.h) \
    $(wildcard include/config/psi.h) \
    $(wildcard include/config/compat/brk.h) \
    $(wildcard include/config/cgroups.h) \
    $(wildcard include/config/blk/cgroup.h) \
    $(wildcard include/config/arch/has/scaled/cputime.h) \
    $(wildcard include/config/virt/cpu/accounting/gen.h) \
    $(wildcard include/config/posix/cputimers.h) \
    $(wildcard include/config/keys.h) \
    $(wildcard include/config/sysvipc.h) \
    $(wildcard include/config/detect/hung/task.h) \
    $(wildcard include/config/audit.h) \
    $(wildcard include/config/auditsyscall.h) \
    $(wildcard include/config/rt/mutexes.h) \
    $(wildcard include/config/ubsan.h) \
    $(wildcard include/config/block.h) \
    $(wildcard include/config/task/xacct.h) \
    $(wildcard include/config/cpusets.h) \
    $(wildcard include/config/x86/cpu/resctrl.h) \
    $(wildcard include/config/futex.h) \
    $(wildcard include/config/perf/events.h) \
    $(wildcard include/config/rseq.h) \
    $(wildcard include/config/task/delay/acct.h) \
    $(wildcard include/config/fault/injection.h) \
    $(wildcard include/config/latencytop.h) \
    $(wildcard include/config/function/graph/tracer.h) \
    $(wildcard include/config/kcov.h) \
    $(wildcard include/config/bcache.h) \
    $(wildcard include/config/vmap/stack.h) \
    $(wildcard include/config/livepatch.h) \
    $(wildcard include/config/security.h) \
    $(wildcard include/config/gcc/plugin/stackleak.h) \
    $(wildcard include/config/arch/task/struct/on/stack.h) \
    $(wildcard include/config/debug/rseq.h) \
  include/uapi/linux/sched.h \
  include/linux/pid.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/rculist.h \
  include/linux/rculist.h \
    $(wildcard include/config/prove/rcu/list.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/refcount.h \
  include/linux/refcount.h \
    $(wildcard include/config/refcount/full.h) \
  arch/x86/include/asm/refcount.h \
  include/linux/sem.h \
  include/uapi/linux/sem.h \
  include/linux/ipc.h \
  include/linux/uidgid.h \
    $(wildcard include/config/multiuser.h) \
    $(wildcard include/config/user/ns.h) \
  include/linux/highuid.h \
  include/linux/rhashtable-types.h \
  include/uapi/linux/ipc.h \
  arch/x86/include/generated/uapi/asm/ipcbuf.h \
  include/uapi/asm-generic/ipcbuf.h \
  arch/x86/include/uapi/asm/sembuf.h \
  include/linux/shm.h \
  include/uapi/linux/shm.h \
  include/uapi/asm-generic/hugetlb_encode.h \
  arch/x86/include/uapi/asm/shmbuf.h \
  include/uapi/asm-generic/shmbuf.h \
  arch/x86/include/asm/shmparam.h \
  include/linux/kcov.h \
  include/uapi/linux/kcov.h \
  include/linux/plist.h \
    $(wildcard include/config/debug/plist.h) \
  include/linux/hrtimer.h \
    $(wildcard include/config/high/res/timers.h) \
    $(wildcard include/config/time/low/res.h) \
    $(wildcard include/config/timerfd.h) \
  include/linux/hrtimer_defs.h \
  include/linux/timerqueue.h \
  include/linux/seccomp.h \
    $(wildcard include/config/seccomp.h) \
    $(wildcard include/config/have/arch/seccomp/filter.h) \
    $(wildcard include/config/seccomp/filter.h) \
    $(wildcard include/config/checkpoint/restore.h) \
  include/uapi/linux/seccomp.h \
  arch/x86/include/asm/seccomp.h \
  arch/x86/include/asm/unistd.h \
  arch/x86/include/uapi/asm/unistd.h \
  arch/x86/include/generated/uapi/asm/unistd_64.h \
  arch/x86/include/generated/asm/unistd_64_x32.h \
  arch/x86/include/asm/ia32_unistd.h \
  arch/x86/include/generated/asm/unistd_32_ia32.h \
  include/asm-generic/seccomp.h \
  include/uapi/linux/unistd.h \
  include/linux/resource.h \
  include/uapi/linux/resource.h \
  arch/x86/include/generated/uapi/asm/resource.h \
  include/asm-generic/resource.h \
  include/uapi/asm-generic/resource.h \
  include/linux/latencytop.h \
  include/linux/sched/prio.h \
  include/linux/sched/types.h \
  include/linux/signal_types.h \
    $(wildcard include/config/old/sigaction.h) \
  include/uapi/linux/signal.h \
  arch/x86/include/asm/signal.h \
  arch/x86/include/uapi/asm/signal.h \
  include/uapi/asm-generic/signal-defs.h \
  arch/x86/include/uapi/asm/siginfo.h \
  include/uapi/asm-generic/siginfo.h \
  include/linux/task_io_accounting.h \
    $(wildcard include/config/task/io/accounting.h) \
  include/linux/posix-timers.h \
    $(wildcard include/config/posix/timers.h) \
  include/linux/alarmtimer.h \
  include/uapi/linux/rseq.h \
  arch/x86/include/asm/uaccess.h \
    $(wildcard include/config/x86/intel/usercopy.h) \
  arch/x86/include/asm/smap.h \
  arch/x86/include/asm/extable.h \
  arch/x86/include/asm/uaccess_64.h \
  include/uapi/linux/uio.h \
  include/uapi/linux/socket.h \
  include/uapi/linux/hdlc/ioctl.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/compat_autoconf.h \
    $(wildcard include/config/compat/if/iscsi/scsi/req.h) \
    $(wildcard include/config/compat/scsi/target/unblock.h) \
    $(wildcard include/config/compat/eth/hw/addr/random.h) \
    $(wildcard include/config/compat/netdev/features.h) \
    $(wildcard include/config/compat/is/work/busy.h) \
    $(wildcard include/config/compat/is/maxrate.h) \
    $(wildcard include/config/compat/is/qcn.h) \
    $(wildcard include/config/compat/netif/is/bond/master.h) \
    $(wildcard include/config/compat/netif/is/xps.h) \
    $(wildcard include/config/compat/netif/has/set/xps/queue.h) \
    $(wildcard include/config/compat/sock/has/queue.h) \
    $(wildcard include/config/compat/skb/has/frag/list.h) \
    $(wildcard include/config/compat/has/irq/affinity/notifier.h) \
    $(wildcard include/config/compat/netif/f/rxhash.h) \
    $(wildcard include/config/compat/is/linux/cpu/rmap.h) \
    $(wildcard include/config/compat/rcu.h) \
    $(wildcard include/config/compat/is/kstrtox.h) \
    $(wildcard include/config/compat/is/ip/tos2prio.h) \
    $(wildcard include/config/compat/is/bitop.h) \
    $(wildcard include/config/compat/dev/uc/mc/add/const.h) \
    $(wildcard include/config/compat/ndo/vf/mac/vlan.h) \
    $(wildcard include/config/compat/is/vf/info/spoofchk.h) \
    $(wildcard include/config/compat/is/vf/info/linkstate.h) \
    $(wildcard include/config/compat/is/pci/physfn.h) \
    $(wildcard include/config/compat/xprt/reserve/xprt/cong/2params.h) \
    $(wildcard include/config/compat/reserve/xprt/2params.h) \
    $(wildcard include/config/compat/xprt/alloc/4params.h) \
    $(wildcard include/config/compat/xprtrdma/needed.h) \
    $(wildcard include/config/compat/virtqueue/get/buf.h) \
    $(wildcard include/config/compat/virtqueue/kick.h) \
    $(wildcard include/config/compat/zc/request.h) \
    $(wildcard include/config/compat/gfp/t.h) \
    $(wildcard include/config/compat/is/bitmap.h) \
    $(wildcard include/config/compat/is/blkdev/issue/zeroout/has/flags.h) \
    $(wildcard include/config/compat/is/prio/tc/map.h) \
    $(wildcard include/config/is/rx/handler/result.h) \
    $(wildcard include/config/is/ndo/add/slave.h) \
    $(wildcard include/config/module/eeprom/ethtool.h) \
    $(wildcard include/config/timestamp/ethtool.h) \
    $(wildcard include/config/compat/indir/setting.h) \
    $(wildcard include/config/compat/en/sysfs.h) \
    $(wildcard include/config/compat/vxlan/enabled.h) \
    $(wildcard include/config/compat/ifla/vf/link/state/max.h) \
    $(wildcard include/config/compat/vlan/egress/visible.h) \
    $(wildcard include/config/genetlink/is/list/head.h) \
    $(wildcard include/config/compat/setnumtcs/int.h) \
    $(wildcard include/config/compat/fdb/api/exists.h) \
    $(wildcard include/config/compat/fdb/add/nlattr.h) \
    $(wildcard include/config/compat/fdb/del/nlattr.h) \
    $(wildcard include/config/compat/fdb/const/addr.h) \
    $(wildcard include/config/compat/fdb/u16/vid.h) \
    $(wildcard include/config/compat/select/queue/fallback.h) \
    $(wildcard include/config/compat/ptp/clock.h) \
    $(wildcard include/config/compat/ptp/n/pins.h) \
    $(wildcard include/config/compat/ptp/clock/register.h) \
    $(wildcard include/config/compat/this.h) \
    $(wildcard include/config/compat/is/vf/info/max/tx/rate.h) \
    $(wildcard include/config/compat/net/device/is/num/tc.h) \
    $(wildcard include/config/compat/is/filter/dev.h) \
    $(wildcard include/config/compat/is/int/setapp.h) \
    $(wildcard include/config/compat/is/enum/pcie/link/width.h) \
    $(wildcard include/config/compat/svc/xprt/ops/xpo/secure/port.h) \
    $(wildcard include/config/compat/is/irq/desc/get/irq/data.h) \
    $(wildcard include/config/compat/is/svc/xprt/class/xcl/ident.h) \
    $(wildcard include/config/compat/is/xprt/create/net.h) \
    $(wildcard include/config/compat/is/rpc/rqst/rq/xmit/bytes/sent.h) \
    $(wildcard include/config/compat/is/sunrpc/addr/h.h) \
    $(wildcard include/config/compat/is/inet/sock/inet/num.h) \
    $(wildcard include/config/compat/is/sock/sk/wq.h) \
    $(wildcard include/config/compat/is/netdev/dql.h) \
    $(wildcard include/config/compat/is/get/tunable.h) \
    $(wildcard include/config/compat/scsi/transfer/length.h) \
    $(wildcard include/config/compat/is/smp/mb/after/atomic.h) \
    $(wildcard include/config/compat/iscsit/wait/conn.h) \
    $(wildcard include/config/compat/iscsit/aborted/task.h) \
    $(wildcard include/config/compat/iscsit/get/sup/prot/ops.h) \
    $(wildcard include/config/compat/iscsit/priv/cmd.h) \
    $(wildcard include/config/compat/is/file/inode.h) \
    $(wildcard include/config/compat/is/this/cpu/ptr.h) \
    $(wildcard include/config/compat/is/const/pci/error/handlers.h) \
    $(wildcard include/config/compat/is/dcbnl/ops/const.h) \
    $(wildcard include/config/compat/tc/prio/is/major.h) \
    $(wildcard include/config/mlx5/tc/ct.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/module.h \
    $(wildcard include/config/dtrace.h) \
    $(wildcard include/config/ctf.h) \
  include/linux/module.h \
    $(wildcard include/config/modules/tree/lookup.h) \
    $(wildcard include/config/module/sig.h) \
    $(wildcard include/config/kallsyms.h) \
    $(wildcard include/config/bpf/events.h) \
    $(wildcard include/config/event/tracing.h) \
    $(wildcard include/config/module/unload.h) \
    $(wildcard include/config/constructors.h) \
    $(wildcard include/config/function/error/injection.h) \
  include/linux/stat.h \
  arch/x86/include/uapi/asm/stat.h \
  include/uapi/linux/stat.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/kmod.h \
    $(wildcard include/config/mlnx/block/request.h) \
  include/linux/kmod.h \
  include/linux/umh.h \
  include/linux/sysctl.h \
    $(wildcard include/config/sysctl.h) \
  include/uapi/linux/sysctl.h \
  include/linux/elf.h \
  arch/x86/include/asm/elf.h \
    $(wildcard include/config/x86/x32/abi.h) \
  arch/x86/include/asm/user.h \
  arch/x86/include/asm/user_64.h \
  arch/x86/include/asm/fsgsbase.h \
  arch/x86/include/asm/vdso.h \
    $(wildcard include/config/x86/x32.h) \
  include/uapi/linux/elf.h \
  include/uapi/linux/elf-em.h \
  include/linux/kobject.h \
    $(wildcard include/config/uevent/helper.h) \
    $(wildcard include/config/debug/kobject/release.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/sysfs.h \
  include/linux/sysfs.h \
  include/linux/kernfs.h \
    $(wildcard include/config/kernfs.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/idr.h \
  include/linux/idr.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/radix-tree.h \
  include/linux/radix-tree.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/xarray.h \
    $(wildcard include/config/xarray/multi.h) \
  include/linux/xarray.h \
  include/linux/kobject_ns.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/kref.h \
  include/linux/kref.h \
  include/linux/moduleparam.h \
    $(wildcard include/config/alpha.h) \
    $(wildcard include/config/ia64.h) \
    $(wildcard include/config/ppc64.h) \
  include/linux/rbtree_latch.h \
  include/linux/error-injection.h \
  include/asm-generic/error-injection.h \
  arch/x86/include/asm/module.h \
    $(wildcard include/config/unwinder/orc.h) \
    $(wildcard include/config/m486.h) \
    $(wildcard include/config/m586.h) \
    $(wildcard include/config/m586tsc.h) \
    $(wildcard include/config/m586mmx.h) \
    $(wildcard include/config/mcore2.h) \
    $(wildcard include/config/m686.h) \
    $(wildcard include/config/mpentiumii.h) \
    $(wildcard include/config/mpentiumiii.h) \
    $(wildcard include/config/mpentiumm.h) \
    $(wildcard include/config/mpentium4.h) \
    $(wildcard include/config/mk6.h) \
    $(wildcard include/config/mk8.h) \
    $(wildcard include/config/melan.h) \
    $(wildcard include/config/mcrusoe.h) \
    $(wildcard include/config/mefficeon.h) \
    $(wildcard include/config/mwinchipc6.h) \
    $(wildcard include/config/mwinchip3d.h) \
    $(wildcard include/config/mcyrixiii.h) \
    $(wildcard include/config/mviac3/2.h) \
    $(wildcard include/config/mviac7.h) \
    $(wildcard include/config/mgeodegx1.h) \
    $(wildcard include/config/mgeode/lx.h) \
  include/asm-generic/module.h \
    $(wildcard include/config/have/mod/arch/specific.h) \
    $(wildcard include/config/modules/use/elf/rel.h) \
    $(wildcard include/config/modules/use/elf/rela.h) \
  arch/x86/include/asm/orc_types.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/infiniband/debug/mtrack.h \
    $(wildcard include/config/generic/ioremap.h) \
    $(wildcard include/config/ppc.h) \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/infiniband/debug/memtrack.h \
  include/linux/io.h \
    $(wildcard include/config/have/arch/huge/vmap.h) \
  include/linux/random.h \
    $(wildcard include/config/arch/random.h) \
  include/linux/once.h \
  include/uapi/linux/random.h \
  include/linux/irqnr.h \
  include/uapi/linux/irqnr.h \
  include/linux/prandom.h \
  arch/x86/include/asm/archrandom.h \
  include/linux/io-mapping.h \
    $(wildcard include/config/have/atomic/iomap.h) \
  arch/x86/include/asm/pgtable.h \
    $(wildcard include/config/debug/wx.h) \
    $(wildcard include/config/have/arch/transparent/hugepage/pud.h) \
    $(wildcard include/config/arch/has/pte/devmap.h) \
    $(wildcard include/config/have/arch/soft/dirty.h) \
    $(wildcard include/config/arch/enable/thp/migration.h) \
  arch/x86/include/asm/fpu/xstate.h \
  arch/x86/include/asm/fpu/api.h \
    $(wildcard include/config/x86/debug/fpu.h) \
  arch/x86/include/asm/pgtable_64.h \
  arch/x86/include/asm/pgtable-invert.h \
  include/asm-generic/pgtable.h \
    $(wildcard include/config/x86/espfix64.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mm.h \
  include/linux/mm.h \
    $(wildcard include/config/have/arch/mmap/rnd/bits.h) \
    $(wildcard include/config/have/arch/mmap/rnd/compat/bits.h) \
    $(wildcard include/config/arch/uses/high/vma/flags.h) \
    $(wildcard include/config/arch/has/pkeys.h) \
    $(wildcard include/config/parisc.h) \
    $(wildcard include/config/sparc64.h) \
    $(wildcard include/config/stack/growsup.h) \
    $(wildcard include/config/shmem.h) \
    $(wildcard include/config/dev/pagemap/ops.h) \
    $(wildcard include/config/device/private.h) \
    $(wildcard include/config/pci/p2pdma.h) \
    $(wildcard include/config/debug/vm/rb.h) \
    $(wildcard include/config/page/poisoning.h) \
    $(wildcard include/config/init/on/alloc/default/on.h) \
    $(wildcard include/config/init/on/free/default/on.h) \
    $(wildcard include/config/debug/pagealloc.h) \
    $(wildcard include/config/arch/has/set/direct/map.h) \
    $(wildcard include/config/hibernation.h) \
    $(wildcard include/config/hugetlbfs.h) \
  include/linux/bit_spinlock.h \
  include/linux/shrinker.h \
  include/linux/page_ext.h \
  include/linux/stacktrace.h \
    $(wildcard include/config/stacktrace.h) \
    $(wildcard include/config/arch/stackwalk.h) \
    $(wildcard include/config/have/reliable/stacktrace.h) \
  include/linux/stackdepot.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/page_ref.h \
  include/linux/page_ref.h \
    $(wildcard include/config/debug/page/ref.h) \
  include/linux/memremap.h \
  include/linux/ioport.h \
    $(wildcard include/config/io/strict/devmem.h) \
  include/linux/sizes.h \
  include/linux/huge_mm.h \
  include/linux/sched/coredump.h \
    $(wildcard include/config/core/dump/default/elf/headers.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/fs.h \
  include/linux/fs.h \
    $(wildcard include/config/read/only/thp/for/fs.h) \
    $(wildcard include/config/fs/posix/acl.h) \
    $(wildcard include/config/cgroup/writeback.h) \
    $(wildcard include/config/ima.h) \
    $(wildcard include/config/file/locking.h) \
    $(wildcard include/config/fsnotify.h) \
    $(wildcard include/config/fs/encryption.h) \
    $(wildcard include/config/fs/verity.h) \
    $(wildcard include/config/epoll.h) \
    $(wildcard include/config/quota.h) \
    $(wildcard include/config/blk/dev/loop.h) \
    $(wildcard include/config/fs/dax.h) \
    $(wildcard include/config/mandatory/file/locking.h) \
    $(wildcard include/config/migration.h) \
    $(wildcard include/config/io/uring.h) \
  include/linux/wait_bit.h \
  include/linux/kdev_t.h \
  include/uapi/linux/kdev_t.h \
  include/linux/dcache.h \
  include/linux/rculist_bl.h \
  include/linux/list_bl.h \
  include/linux/lockref.h \
    $(wildcard include/config/arch/use/cmpxchg/lockref.h) \
  include/linux/stringhash.h \
    $(wildcard include/config/dcache/word/access.h) \
  include/linux/hash.h \
    $(wildcard include/config/have/arch/hash.h) \
  include/linux/path.h \
  include/linux/list_lru.h \
  include/linux/capability.h \
  include/uapi/linux/capability.h \
  include/linux/semaphore.h \
  include/linux/fcntl.h \
    $(wildcard include/config/arch/32bit/off/t.h) \
  include/uapi/linux/fcntl.h \
  arch/x86/include/generated/uapi/asm/fcntl.h \
  include/uapi/asm-generic/fcntl.h \
  include/uapi/linux/fiemap.h \
  include/linux/migrate_mode.h \
  include/linux/percpu-rwsem.h \
  include/linux/rcuwait.h \
  include/linux/rcu_sync.h \
  include/linux/delayed_call.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/uuid.h \
  include/linux/uuid.h \
  include/uapi/linux/uuid.h \
  include/linux/errseq.h \
  include/linux/ioprio.h \
  include/linux/sched/rt.h \
  include/linux/iocontext.h \
  include/linux/fs_types.h \
  include/uapi/linux/fs.h \
  include/linux/quota.h \
    $(wildcard include/config/quota/netlink/interface.h) \
  include/linux/percpu_counter.h \
  include/uapi/linux/dqblk_xfs.h \
  include/linux/dqblk_v1.h \
  include/linux/dqblk_v2.h \
  include/linux/dqblk_qtree.h \
  include/linux/projid.h \
  include/uapi/linux/quota.h \
  include/linux/nfs_fs_i.h \
  include/linux/vmstat.h \
    $(wildcard include/config/vm/event/counters.h) \
    $(wildcard include/config/debug/tlbflush.h) \
    $(wildcard include/config/debug/vm/vmacache.h) \
  include/linux/vm_event_item.h \
    $(wildcard include/config/memory/balloon.h) \
    $(wildcard include/config/balloon/compaction.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/scatterlist.h \
  include/linux/scatterlist.h \
    $(wildcard include/config/need/sg/dma/length.h) \
    $(wildcard include/config/debug/sg.h) \
    $(wildcard include/config/sgl/alloc.h) \
    $(wildcard include/config/arch/no/sg/chain.h) \
    $(wildcard include/config/sg/pool.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/skbuff.h \
  include/linux/skbuff.h \
    $(wildcard include/config/nf/conntrack.h) \
    $(wildcard include/config/bridge/netfilter.h) \
    $(wildcard include/config/net/tc/skb/ext.h) \
    $(wildcard include/config/skb/extensions.h) \
    $(wildcard include/config/ipv6/ndisc/nodetype.h) \
    $(wildcard include/config/net/switchdev.h) \
    $(wildcard include/config/net/cls/act.h) \
    $(wildcard include/config/net/redirect.h) \
    $(wildcard include/config/tls/device.h) \
    $(wildcard include/config/net/sched.h) \
    $(wildcard include/config/net/rx/busy/poll.h) \
    $(wildcard include/config/xps.h) \
    $(wildcard include/config/network/secmark.h) \
    $(wildcard include/config/net.h) \
    $(wildcard include/config/have/efficient/unaligned/access.h) \
    $(wildcard include/config/network/phy/timestamping.h) \
    $(wildcard include/config/xfrm.h) \
    $(wildcard include/config/netfilter/xt/target/trace.h) \
    $(wildcard include/config/nf/tables.h) \
    $(wildcard include/config/ip/vs.h) \
  include/linux/bvec.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/net.h \
  include/linux/net.h \
  include/uapi/linux/net.h \
  include/linux/textsearch.h \
  include/net/checksum.h \
  arch/x86/include/asm/checksum.h \
  arch/x86/include/asm/checksum_64.h \
  include/linux/dma-mapping.h \
    $(wildcard include/config/dma/declare/coherent.h) \
    $(wildcard include/config/arch/has/sync/dma/for/device.h) \
    $(wildcard include/config/swiotlb.h) \
    $(wildcard include/config/arch/has/sync/dma/for/cpu.h) \
    $(wildcard include/config/arch/has/sync/dma/for/cpu/all.h) \
    $(wildcard include/config/has/dma.h) \
    $(wildcard include/config/arch/has/setup/dma/ops.h) \
    $(wildcard include/config/arch/has/teardown/dma/ops.h) \
    $(wildcard include/config/need/dma/map/state.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/device.h \
  include/linux/device.h \
    $(wildcard include/config/debug/devres.h) \
    $(wildcard include/config/generic/msi/irq/domain.h) \
    $(wildcard include/config/pinctrl.h) \
    $(wildcard include/config/generic/msi/irq.h) \
    $(wildcard include/config/dma/cma.h) \
    $(wildcard include/config/of.h) \
    $(wildcard include/config/devtmpfs.h) \
    $(wildcard include/config/sysfs/deprecated.h) \
  include/linux/klist.h \
  include/linux/pm.h \
    $(wildcard include/config/vt/console/sleep.h) \
    $(wildcard include/config/pm.h) \
    $(wildcard include/config/pm/clk.h) \
    $(wildcard include/config/pm/generic/domains.h) \
  include/linux/ratelimit.h \
  arch/x86/include/asm/device.h \
    $(wildcard include/config/intel/iommu.h) \
    $(wildcard include/config/amd/iommu.h) \
    $(wildcard include/config/sta2x11.h) \
    $(wildcard include/config/x86/dev/dma/ops.h) \
    $(wildcard include/config/pci/domains.h) \
  include/linux/pm_wakeup.h \
  include/linux/dma-debug.h \
    $(wildcard include/config/dma/api/debug.h) \
  include/linux/dma-direction.h \
  arch/x86/include/asm/dma-mapping.h \
  arch/x86/include/asm/swiotlb.h \
  include/linux/swiotlb.h \
  include/linux/dma-contiguous.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/netdev_features.h \
  include/linux/netdev_features.h \
  include/linux/sched/clock.h \
    $(wildcard include/config/have/unstable/sched/clock.h) \
    $(wildcard include/config/irq/time/accounting.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/flow_dissector.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/../../compat/config.h \
  include/net/flow_dissector.h \
  include/linux/in6.h \
  include/uapi/linux/in6.h \
  include/linux/siphash.h \
  include/uapi/linux/if_ether.h \
  include/linux/splice.h \
  include/linux/pipe_fs_i.h \
  include/uapi/linux/if_packet.h \
  include/net/flow.h \
  include/linux/netfilter/nf_conntrack_common.h \
  include/uapi/linux/netfilter/nf_conntrack_common.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/pci.h \
  include/linux/pci.h \
    $(wildcard include/config/pci/iov.h) \
    $(wildcard include/config/pcieaer.h) \
    $(wildcard include/config/pcieaspm.h) \
    $(wildcard include/config/hotplug/pci/pcie.h) \
    $(wildcard include/config/pcie/ptm.h) \
    $(wildcard include/config/pci/ats.h) \
    $(wildcard include/config/pci/pri.h) \
    $(wildcard include/config/pci/pasid.h) \
    $(wildcard include/config/pci/domains/generic.h) \
    $(wildcard include/config/pcieportbus.h) \
    $(wildcard include/config/pci/quirks.h) \
    $(wildcard include/config/hibernate/callbacks.h) \
    $(wildcard include/config/acpi/mcfg.h) \
    $(wildcard include/config/hotplug/pci.h) \
    $(wildcard include/config/eeh.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mod_devicetable.h \
  include/uapi/linux/mei.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/interrupt.h \
  include/linux/interrupt.h \
    $(wildcard include/config/irq/forced/threading.h) \
    $(wildcard include/config/generic/irq/probe.h) \
    $(wildcard include/config/irq/timings.h) \
  include/linux/irqreturn.h \
  include/linux/hardirq.h \
  include/linux/ftrace_irq.h \
    $(wildcard include/config/ftrace/nmi/enter.h) \
    $(wildcard include/config/hwlat/tracer.h) \
  include/linux/vtime.h \
    $(wildcard include/config/virt/cpu/accounting.h) \
  include/linux/context_tracking_state.h \
    $(wildcard include/config/context/tracking.h) \
  arch/x86/include/asm/irq.h \
  arch/x86/include/asm/sections.h \
  include/asm-generic/sections.h \
  include/linux/resource_ext.h \
  include/uapi/linux/pci.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/pci_regs.h \
  include/uapi/linux/pci_regs.h \
  include/linux/pci_ids.h \
  include/linux/dmapool.h \
  arch/x86/include/asm/pci.h \
    $(wildcard include/config/pci/msi/irq/domain.h) \
    $(wildcard include/config/vmd.h) \
  arch/x86/include/asm/pat.h \
  arch/x86/include/asm/pci_64.h \
    $(wildcard include/config/calgary/iommu.h) \
  include/asm-generic/pci.h \
  include/linux/pci-dma-compat.h \
  include/linux/compiler_types.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_accel/ktls_txrx.h \
    $(wildcard include/config/mlx5/en/tls.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/tls.h \
  include/net/tls.h \
    $(wildcard include/config/sock/validate/xmit.h) \
  include/linux/tcp.h \
    $(wildcard include/config/smc.h) \
    $(wildcard include/config/bpf.h) \
    $(wildcard include/config/tcp/md5sig.h) \
  include/linux/win_minmax.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/sock.h \
  include/net/sock.h \
    $(wildcard include/config/ipv6.h) \
    $(wildcard include/config/bpf/syscall.h) \
    $(wildcard include/config/rps.h) \
    $(wildcard include/config/sock/cgroup/data.h) \
    $(wildcard include/config/inet.h) \
  include/linux/list_nulls.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/netdevice.h \
    $(wildcard include/config/net/cls/e2e/cache.h) \
    $(wildcard include/config/bql.h) \
  include/linux/netdevice.h \
    $(wildcard include/config/dcb.h) \
    $(wildcard include/config/hyperv/net.h) \
    $(wildcard include/config/wlan.h) \
    $(wildcard include/config/ax25.h) \
    $(wildcard include/config/mac80211/mesh.h) \
    $(wildcard include/config/net/ipip.h) \
    $(wildcard include/config/net/ipgre.h) \
    $(wildcard include/config/ipv6/sit.h) \
    $(wildcard include/config/ipv6/tunnel.h) \
    $(wildcard include/config/netpoll.h) \
    $(wildcard include/config/xdp/sockets.h) \
    $(wildcard include/config/rfs/accel.h) \
    $(wildcard include/config/fcoe.h) \
    $(wildcard include/config/xfrm/offload.h) \
    $(wildcard include/config/net/poll/controller.h) \
    $(wildcard include/config/libfcoe.h) \
    $(wildcard include/config/wireless/ext.h) \
    $(wildcard include/config/net/l3/master/dev.h) \
    $(wildcard include/config/vlan/8021q.h) \
    $(wildcard include/config/net/dsa.h) \
    $(wildcard include/config/tipc.h) \
    $(wildcard include/config/irda.h) \
    $(wildcard include/config/atalk.h) \
    $(wildcard include/config/decnet.h) \
    $(wildcard include/config/mpls/routing.h) \
    $(wildcard include/config/netfilter/ingress.h) \
    $(wildcard include/config/garp.h) \
    $(wildcard include/config/mrp.h) \
    $(wildcard include/config/cgroup/net/prio.h) \
    $(wildcard include/config/net/flow/limit.h) \
  include/linux/delay.h \
  arch/x86/include/asm/delay.h \
  include/asm-generic/delay.h \
  include/linux/prefetch.h \
  include/linux/dynamic_queue_limits.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/ethtool.h \
  include/linux/ethtool.h \
  include/linux/compat.h \
    $(wildcard include/config/arch/has/syscall/wrapper.h) \
    $(wildcard include/config/compat/old/sigaction.h) \
    $(wildcard include/config/odd/rt/sigaction.h) \
  include/uapi/linux/aio_abi.h \
  arch/x86/include/asm/compat.h \
  include/linux/sched/task_stack.h \
    $(wildcard include/config/debug/stack/usage.h) \
  include/uapi/linux/magic.h \
  arch/x86/include/asm/user32.h \
  include/asm-generic/compat.h \
  arch/x86/include/asm/syscall_wrapper.h \
  include/uapi/linux/ethtool.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/if_ether.h \
  include/linux/if_ether.h \
  include/net/net_namespace.h \
    $(wildcard include/config/ieee802154/6lowpan.h) \
    $(wildcard include/config/ip/sctp.h) \
    $(wildcard include/config/ip/dccp.h) \
    $(wildcard include/config/netfilter.h) \
    $(wildcard include/config/nf/defrag/ipv6.h) \
    $(wildcard include/config/netfilter/netlink/acct.h) \
    $(wildcard include/config/nf/ct/netlink/timeout.h) \
    $(wildcard include/config/wext/core.h) \
    $(wildcard include/config/mpls.h) \
    $(wildcard include/config/can.h) \
    $(wildcard include/config/crypto/user.h) \
    $(wildcard include/config/net/ns.h) \
  include/net/netns/core.h \
  include/net/netns/mib.h \
    $(wildcard include/config/xfrm/statistics.h) \
  include/net/snmp.h \
  include/uapi/linux/snmp.h \
  include/linux/u64_stats_sync.h \
  include/net/netns/unix.h \
  include/net/netns/packet.h \
  include/net/netns/ipv4.h \
    $(wildcard include/config/ip/multiple/tables.h) \
    $(wildcard include/config/ip/route/classid.h) \
    $(wildcard include/config/ip/mroute.h) \
    $(wildcard include/config/ip/mroute/multiple/tables.h) \
    $(wildcard include/config/ip/route/multipath.h) \
  include/net/inet_frag.h \
  include/net/netns/ipv6.h \
    $(wildcard include/config/ipv6/multiple/tables.h) \
    $(wildcard include/config/ipv6/mroute.h) \
    $(wildcard include/config/ipv6/mroute/multiple/tables.h) \
  include/net/dst_ops.h \
  include/uapi/linux/icmpv6.h \
  include/net/netns/nexthop.h \
  include/net/netns/ieee802154_6lowpan.h \
  include/net/netns/sctp.h \
  include/net/netns/dccp.h \
  include/net/netns/netfilter.h \
    $(wildcard include/config/netfilter/family/arp.h) \
    $(wildcard include/config/netfilter/family/bridge.h) \
    $(wildcard include/config/nf/defrag/ipv4.h) \
  include/linux/netfilter_defs.h \
  include/uapi/linux/netfilter.h \
  include/linux/in.h \
  include/uapi/linux/in.h \
  include/net/netns/x_tables.h \
    $(wildcard include/config/bridge/nf/ebtables.h) \
  include/net/netns/conntrack.h \
    $(wildcard include/config/nf/ct/proto/dccp.h) \
    $(wildcard include/config/nf/ct/proto/sctp.h) \
    $(wildcard include/config/nf/ct/proto/gre.h) \
    $(wildcard include/config/nf/conntrack/events.h) \
    $(wildcard include/config/nf/conntrack/labels.h) \
  include/linux/netfilter/nf_conntrack_tcp.h \
  include/uapi/linux/netfilter/nf_conntrack_tcp.h \
  include/linux/netfilter/nf_conntrack_dccp.h \
  include/uapi/linux/netfilter/nf_conntrack_tuple_common.h \
  include/linux/netfilter/nf_conntrack_sctp.h \
  include/uapi/linux/netfilter/nf_conntrack_sctp.h \
  include/net/netns/nftables.h \
  include/net/netns/xfrm.h \
  include/uapi/linux/xfrm.h \
  include/net/netns/mpls.h \
  include/net/netns/can.h \
  include/net/netns/xdp.h \
  include/linux/ns_common.h \
  include/linux/seq_file_net.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/seq_file.h \
  include/linux/seq_file.h \
  include/linux/cred.h \
    $(wildcard include/config/debug/credentials.h) \
  include/linux/key.h \
  include/linux/assoc_array.h \
    $(wildcard include/config/associative/array.h) \
  include/linux/sched/user.h \
    $(wildcard include/config/fanotify.h) \
    $(wildcard include/config/posix/mqueue.h) \
  include/net/dcbnl.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/dcbnl.h \
  include/uapi/linux/dcbnl.h \
  include/net/netprio_cgroup.h \
  include/linux/cgroup.h \
    $(wildcard include/config/cgroup/cpuacct.h) \
    $(wildcard include/config/cgroup/net/classid.h) \
    $(wildcard include/config/cgroup/data.h) \
    $(wildcard include/config/cgroup/bpf.h) \
  include/uapi/linux/cgroupstats.h \
  include/uapi/linux/taskstats.h \
  include/linux/nsproxy.h \
  include/linux/user_namespace.h \
    $(wildcard include/config/inotify/user.h) \
    $(wildcard include/config/persistent/keyrings.h) \
  include/linux/kernel_stat.h \
  include/linux/cgroup-defs.h \
  include/linux/bpf-cgroup.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/bpf.h \
  include/linux/bpf.h \
    $(wildcard include/config/bpf/jit/always/on.h) \
    $(wildcard include/config/bpf/stream/parser.h) \
  include/uapi/linux/bpf.h \
    $(wildcard include/config/efficient/unaligned/access.h) \
    $(wildcard include/config/bpf/kprobe/override.h) \
    $(wildcard include/config/bpf/lirc/mode2.h) \
  include/uapi/linux/bpf_common.h \
  include/linux/file.h \
  include/linux/bpf_types.h \
  include/linux/psi_types.h \
  include/linux/kthread.h \
  include/linux/cgroup_subsys.h \
    $(wildcard include/config/cgroup/device.h) \
    $(wildcard include/config/cgroup/freezer.h) \
    $(wildcard include/config/cgroup/perf.h) \
    $(wildcard include/config/cgroup/hugetlb.h) \
    $(wildcard include/config/cgroup/pids.h) \
    $(wildcard include/config/cgroup/rdma.h) \
    $(wildcard include/config/cgroup/debug.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/xdp.h \
  include/net/xdp.h \
  include/uapi/linux/neighbour.h \
  include/linux/netlink.h \
  include/net/scm.h \
    $(wildcard include/config/security/network.h) \
  include/linux/security.h \
    $(wildcard include/config/security/infiniband.h) \
    $(wildcard include/config/security/network/xfrm.h) \
    $(wildcard include/config/security/path.h) \
    $(wildcard include/config/securityfs.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/sched/signal.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/sched/../../../compat/config.h \
  include/linux/sched/signal.h \
    $(wildcard include/config/sched/autogroup.h) \
    $(wildcard include/config/bsd/process/acct.h) \
    $(wildcard include/config/taskstats.h) \
  include/linux/signal.h \
  include/linux/sched/jobctl.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/sched/task.h \
  include/linux/sched/task.h \
    $(wildcard include/config/have/copy/thread/tls.h) \
    $(wildcard include/config/have/exit/thread.h) \
    $(wildcard include/config/arch/wants/dynamic/task/struct.h) \
    $(wildcard include/config/have/arch/thread/struct/whitelist.h) \
  include/uapi/linux/netlink.h \
  include/uapi/linux/netdevice.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/if_link.h \
  include/linux/if_link.h \
  include/uapi/linux/if_link.h \
  include/uapi/linux/if_bonding.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/linux/pkt_cls.h \
    $(wildcard include/config/compat/kernel/4/14.h) \
    $(wildcard include/config/compat/cls/flower/4/18/mod.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/linux/../../../compat/config.h \
  include/uapi/linux/pkt_cls.h \
  include/uapi/linux/pkt_sched.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/hashtable.h \
  include/linux/hashtable.h \
  include/linux/page_counter.h \
  include/linux/memcontrol.h \
    $(wildcard include/config/memcg/swap.h) \
  include/linux/vmpressure.h \
  include/linux/eventfd.h \
    $(wildcard include/config/eventfd.h) \
  include/linux/writeback.h \
  include/linux/flex_proportions.h \
  include/linux/backing-dev-defs.h \
    $(wildcard include/config/debug/fs.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/blk_types.h \
  include/linux/blk_types.h \
    $(wildcard include/config/blk/cgroup/iocost.h) \
    $(wildcard include/config/blk/dev/integrity.h) \
  include/linux/blk-cgroup.h \
    $(wildcard include/config/blk/dev/throttling.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/blkdev.h \
  include/linux/blkdev.h \
    $(wildcard include/config/blk/rq/alloc/time.h) \
    $(wildcard include/config/blk/wbt.h) \
    $(wildcard include/config/blk/dev/zoned.h) \
    $(wildcard include/config/blk/dev/bsg.h) \
    $(wildcard include/config/blk/debug/fs.h) \
  include/uapi/linux/major.h \
  include/linux/genhd.h \
    $(wildcard include/config/fail/make/request.h) \
    $(wildcard include/config/solaris/x86/partition.h) \
    $(wildcard include/config/bsd/disklabel.h) \
    $(wildcard include/config/unixware/disklabel.h) \
    $(wildcard include/config/minix/subpartition.h) \
  arch/x86/include/asm/local.h \
  include/linux/pagemap.h \
  include/linux/highmem.h \
  arch/x86/include/asm/cacheflush.h \
  include/asm-generic/cacheflush.h \
  include/linux/hugetlb_inline.h \
  include/linux/mempool.h \
  include/linux/bio.h \
  include/linux/bsg.h \
  include/uapi/linux/bsg.h \
  include/uapi/linux/blkzoned.h \
  include/linux/elevator.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/filter.h \
  include/linux/filter.h \
    $(wildcard include/config/bpf/jit.h) \
    $(wildcard include/config/have/ebpf/jit.h) \
  include/linux/cryptohash.h \
  include/linux/set_memory.h \
    $(wildcard include/config/arch/has/set/memory.h) \
  arch/x86/include/asm/set_memory.h \
  include/asm-generic/set_memory.h \
  include/linux/kallsyms.h \
    $(wildcard include/config/kallsyms/all.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/if_vlan.h \
  include/linux/if_vlan.h \
  include/linux/etherdevice.h \
  arch/x86/include/asm/unaligned.h \
  include/linux/unaligned/access_ok.h \
  include/linux/unaligned/generic.h \
  include/linux/rtnetlink.h \
    $(wildcard include/config/net/ingress.h) \
    $(wildcard include/config/net/egress.h) \
  include/uapi/linux/rtnetlink.h \
  include/uapi/linux/if_addr.h \
  include/uapi/linux/if_vlan.h \
  include/net/sch_generic.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/linux/pkt_cls.h \
  include/net/gen_stats.h \
  include/uapi/linux/gen_stats.h \
  include/net/rtnetlink.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/netlink.h \
  include/net/netlink.h \
  include/net/genetlink.h \
  include/linux/genetlink.h \
  include/uapi/linux/genetlink.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/flow_offload.h \
  include/net/flow_offload.h \
  include/linux/rhashtable.h \
  include/linux/jhash.h \
  include/linux/unaligned/packed_struct.h \
  include/uapi/linux/filter.h \
  include/linux/rculist_nulls.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/poll.h \
  include/linux/poll.h \
  include/uapi/linux/poll.h \
  arch/x86/include/generated/uapi/asm/poll.h \
  include/uapi/asm-generic/poll.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/linux/eventpoll.h \
  include/uapi/linux/eventpoll.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/linux/eventpoll.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/dst.h \
  include/net/dst.h \
  include/net/neighbour.h \
  include/net/tcp_states.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/linux/net_tstamp.h \
  include/uapi/linux/net_tstamp.h \
  include/net/smc.h \
  include/net/l3mdev.h \
  include/net/fib_rules.h \
  include/uapi/linux/fib_rules.h \
  include/net/fib_notifier.h \
  include/net/inet_connection_sock.h \
  include/net/inet_sock.h \
  include/net/request_sock.h \
  include/net/netns/hash.h \
  include/net/inet_timewait_sock.h \
  include/net/timewait_sock.h \
  include/uapi/linux/tcp.h \
  include/linux/skmsg.h \
  include/net/tcp.h \
    $(wildcard include/config/syn/cookies.h) \
  include/net/inet_hashtables.h \
  include/linux/ip.h \
  include/uapi/linux/ip.h \
  include/linux/ipv6.h \
    $(wildcard include/config/ipv6/router/pref.h) \
    $(wildcard include/config/ipv6/route/info.h) \
    $(wildcard include/config/ipv6/optimistic/dad.h) \
    $(wildcard include/config/ipv6/seg6/hmac.h) \
    $(wildcard include/config/ipv6/mip6.h) \
    $(wildcard include/config/ipv6/subtrees.h) \
  include/uapi/linux/ipv6.h \
  include/linux/udp.h \
  include/uapi/linux/udp.h \
  include/net/route.h \
  include/net/inetpeer.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/ipv6.h \
  include/net/ipv6.h \
  include/linux/jump_label_ratelimit.h \
  include/net/if_inet6.h \
  include/net/ndisc.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/ipv6_stubs.h \
  include/net/ipv6_stubs.h \
  include/linux/icmpv6.h \
    $(wildcard include/config/nf/nat.h) \
  include/linux/if_arp.h \
    $(wildcard include/config/firewire/net.h) \
  include/uapi/linux/if_arp.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/ip_fib.h \
    $(wildcard include/config/compat/is/fib/lookup/static/and/extern.h) \
  include/net/ip_fib.h \
  include/net/arp.h \
  include/uapi/linux/in_route.h \
  include/uapi/linux/route.h \
  include/net/sock_reuseport.h \
  include/net/ip.h \
  include/net/lwtunnel.h \
    $(wildcard include/config/lwtunnel.h) \
  include/uapi/linux/lwtunnel.h \
  include/net/inet_ecn.h \
  include/net/dsfield.h \
  include/net/strparser.h \
  include/crypto/aead.h \
  include/uapi/linux/tls.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en.h \
    $(wildcard include/config/compat/lro/enabled/ipoib.h) \
    $(wildcard include/config/pci/hyperv/interface.h) \
    $(wildcard include/config/mlx5/core/en/dcb.h) \
    $(wildcard include/config/mlx5/macsec.h) \
    $(wildcard include/config/mlx5/en/ipsec.h) \
    $(wildcard include/config/mlx5/eswitch.h) \
  include/linux/timecounter.h \
  include/linux/clocksource.h \
    $(wildcard include/config/arch/clocksource/data.h) \
    $(wildcard include/config/clocksource/watchdog.h) \
    $(wildcard include/config/arch/clocksource/init.h) \
    $(wildcard include/config/timer/probe.h) \
  include/linux/of.h \
    $(wildcard include/config/of/dynamic.h) \
    $(wildcard include/config/sparc.h) \
    $(wildcard include/config/of/promtree.h) \
    $(wildcard include/config/of/kobj.h) \
    $(wildcard include/config/of/numa.h) \
    $(wildcard include/config/of/overlay.h) \
  include/linux/property.h \
  arch/x86/include/asm/clocksource.h \
  include/linux/crash_dump.h \
    $(wildcard include/config/crash/dump.h) \
    $(wildcard include/config/proc/vmcore/device/dump.h) \
    $(wildcard include/config/proc/vmcore.h) \
  include/linux/kexec.h \
    $(wildcard include/config/kexec/core.h) \
    $(wildcard include/config/kexec/file.h) \
    $(wildcard include/config/kexec/sig.h) \
    $(wildcard include/config/kexec/elf.h) \
    $(wildcard include/config/ima/kexec.h) \
    $(wildcard include/config/kexec/jump.h) \
  include/linux/crash_core.h \
  include/linux/elfcore.h \
    $(wildcard include/config/um.h) \
  include/linux/user.h \
  include/uapi/linux/elfcore.h \
    $(wildcard include/config/binfmt/elf/fdpic.h) \
  include/linux/ptrace.h \
  include/linux/pid_namespace.h \
    $(wildcard include/config/pid/ns.h) \
  include/uapi/linux/ptrace.h \
  include/uapi/linux/kexec.h \
  arch/x86/include/asm/kexec.h \
  include/linux/proc_fs.h \
    $(wildcard include/config/proc/pid/arch/status.h) \
  include/uapi/linux/vmcore.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/driver.h \
    $(wildcard include/config/mlx5/sf.h) \
    $(wildcard include/config/mlx5/sf/manager.h) \
    $(wildcard include/config/ptp/1588/clock.h) \
    $(wildcard include/config/mlx5/fpga.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/../../../compat/config.h \
  include/linux/irq.h \
    $(wildcard include/config/generic/irq/effective/aff/mask.h) \
    $(wildcard include/config/generic/irq/ipi.h) \
    $(wildcard include/config/irq/domain/hierarchy.h) \
    $(wildcard include/config/generic/irq/migration.h) \
    $(wildcard include/config/generic/pending/irq.h) \
    $(wildcard include/config/hardirqs/sw/resend.h) \
    $(wildcard include/config/generic/irq/legacy/alloc/hwirq.h) \
    $(wildcard include/config/generic/irq/legacy.h) \
    $(wildcard include/config/generic/irq/multi/handler.h) \
  include/linux/irqhandler.h \
  arch/x86/include/asm/irq_regs.h \
  include/linux/irqdesc.h \
    $(wildcard include/config/irq/preflow/fasteoi.h) \
    $(wildcard include/config/generic/irq/debugfs.h) \
    $(wildcard include/config/sparse/irq.h) \
    $(wildcard include/config/handle/domain/irq.h) \
    $(wildcard include/config/irq/domain.h) \
  arch/x86/include/asm/hw_irq.h \
    $(wildcard include/config/hpet/timer.h) \
    $(wildcard include/config/dmar/table.h) \
    $(wildcard include/config/x86/uv.h) \
  include/linux/profile.h \
    $(wildcard include/config/profiling.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/auxiliary_bus.h \
    $(wildcard include/config/auxiliary/bus.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/device.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/rdma/ib_verbs.h \
    $(wildcard include/config/dynamic/debug/core.h) \
    $(wildcard include/config/irq/poll.h) \
    $(wildcard include/config/infiniband/virt/dma.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/rdma/../../compat/config.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/irq_poll.h \
  include/linux/irq_poll.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mmu_notifier.h \
  include/linux/mmu_notifier.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/cgroup_rdma.h \
  include/linux/cgroup_rdma.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/dim.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/rdma/ib_user_verbs.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/rdma/rdma_counter.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/rdma/restrack.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/rdma/rdma_netlink.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/rdma/rdma_netlink.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/rdma/signature.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/rdma/rdma_user_ioctl.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/rdma/ib_user_mad.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/rdma/hfi/hfi1_ioctl.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/rdma/rdma_user_ioctl_cmds.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/rdma/ib_user_ioctl_verbs.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/rdma/ib_user_verbs.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/rdma/ib_verbs_nvmf_def.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/rdma/ib_verbs_nvmf.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/mlx5_ifc.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/mlx5_ifc_fpga.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/bitfield.h \
  include/linux/bitfield.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/doorbell.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/eq.h \
  include/linux/ptp_clock_kernel.h \
  include/linux/pps_kernel.h \
    $(wildcard include/config/ntp/pps.h) \
  include/uapi/linux/pps.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/cdev.h \
  include/linux/cdev.h \
  include/uapi/linux/ptp_clock.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/devlink.h \
  include/net/devlink.h \
    $(wildcard include/config/net/devlink.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/linux/devlink.h \
  include/uapi/linux/devlink.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/qp.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/cq.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/port.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/vport.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/transobj.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/mlx5/fs.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/switchdev.h \
  include/net/switchdev.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/pkt_cls.h \
  include/net/pkt_cls.h \
    $(wildcard include/config/net/cls.h) \
    $(wildcard include/config/net/ematch.h) \
  include/net/act_api.h \
  include/net/pkt_sched.h \
  include/uapi/linux/pkt_sched.h \
  include/net/netns/generic.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/tc_act/tc_tunnel_key.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/tc_act/../../../compat/config.h \
  include/net/tc_act/tc_tunnel_key.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/linux/tc_act/tc_tunnel_key.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/uapi/linux/tc_act/../../../../compat/config.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/dst_metadata.h \
    $(wildcard include/config/net/sched/new.h) \
  include/net/dst_metadata.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/ip_tunnels.h \
  include/net/ip_tunnels.h \
    $(wildcard include/config/dst/cache.h) \
    $(wildcard include/config/ipv6/sit/6rd.h) \
  include/linux/if_tunnel.h \
  include/uapi/linux/if_tunnel.h \
  include/net/gro_cells.h \
  include/net/dst_cache.h \
  include/net/ip6_fib.h \
  include/linux/ipv6_route.h \
  include/uapi/linux/ipv6_route.h \
  include/net/ip6_route.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/net/addrconf.h \
  include/net/addrconf.h \
  include/net/nexthop.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/wq.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/mlx5_core.h \
    $(wildcard include/config/mlx5/sf/cfg.h) \
    $(wildcard include/config/mlx5/core/en.h) \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/firmware.h \
  include/linux/firmware.h \
    $(wildcard include/config/fw/loader.h) \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/lib/devcom.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_stats.h \
    $(wildcard include/config/mlx5/en/arfs.h) \
    $(wildcard include/config/page/pool/stats.h) \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en/dcbnl.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en/fs.h \
    $(wildcard include/config/mlx5/en/rxnfc.h) \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en/mod_hdr.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/lib/fs_ttc.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en/qos.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/lib/hv_vhca.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/lib/clock.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en/rx_res.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en/rqt.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en/tir.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en/fs.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en/rss.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en/selq.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/lib/sd.h \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en/txrx.h \
  /home/lingbo11/zxm/rdma-kerndriver/include/linux/indirect_call_wrapper.h \
    $(wildcard include/config/mitigation/retpoline.h) \
  /home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_accel/ktls_utils.h \

/home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_accel/ktls_txrx.o: $(deps_/home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_accel/ktls_txrx.o)

$(deps_/home/lingbo11/zxm/rdma-kerndriver/drivers/net/ethernet/mellanox/mlx5/core/en_accel/ktls_txrx.o):
