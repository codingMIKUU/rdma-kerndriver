#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0xa4b86400, "module_layout" },
	{ 0x9170d4f7, "dma_direct_unmap_sg" },
	{ 0x31ca542f, "kmalloc_caches" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x3e0ef07a, "nvmet_wq" },
	{ 0x840342c6, "sgl_free" },
	{ 0x79aa04a2, "get_random_bytes" },
	{ 0xdf566a59, "__x86_indirect_thunk_r9" },
	{ 0x2d2f06d, "memtrack_randomize_mem" },
	{ 0x56470118, "__warn_printk" },
	{ 0xa10393b6, "dma_direct_sync_single_for_cpu" },
	{ 0x87b8798d, "sg_next" },
	{ 0xc8a5ba06, "nvmet_sq_init" },
	{ 0x165b145c, "ex_handler_refcount" },
	{ 0x97651e6c, "vmemmap_base" },
	{ 0xb421a321, "pv_ops" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0x3812050a, "_raw_spin_unlock_irqrestore" },
	{ 0xc5850110, "printk" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0x4c9d28b0, "phys_base" },
	{ 0xffb7c514, "ida_free" },
	{ 0x5a921311, "strncmp" },
	{ 0x51cdf94b, "nvmet_register_transport" },
	{ 0x3db35d93, "dma_direct_map_page" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0x6091797f, "synchronize_rcu" },
	{ 0x885de096, "_dev_err" },
	{ 0x42160169, "flush_workqueue" },
	{ 0x1ca975f9, "nvmet_req_init" },
	{ 0x7cd8d75e, "page_offset_base" },
	{ 0x87d16318, "dma_direct_unmap_page" },
	{ 0x589844d9, "_dev_info" },
	{ 0xa005c413, "nvmet_unregister_transport" },
	{ 0x3a13f54a, "sgl_alloc" },
	{ 0xa916b694, "strnlen" },
	{ 0xc0e93c71, "put_device" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x9cb986f2, "vmalloc_base" },
	{ 0xb6a43883, "memtrack_inject_error" },
	{ 0xb8b9f817, "kmalloc_order_trace" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x67235b02, "nvmet_sq_destroy" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xb4823e9, "dev_driver_string" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xb3a666c0, "kmem_cache_alloc_trace" },
	{ 0xdbf17652, "_raw_spin_lock" },
	{ 0x51760917, "_raw_spin_lock_irqsave" },
	{ 0x83be47e0, "get_device" },
	{ 0x37a0cba, "kfree" },
	{ 0x77145184, "dma_direct_map_sg" },
	{ 0x69acdf38, "memcpy" },
	{ 0xfba7ddd2, "match_u64" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0x6d2e97da, "dma_direct_sync_single_for_device" },
	{ 0xf692761b, "nvmet_req_complete" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0xa7d5f92e, "ida_destroy" },
	{ 0xdf9208c0, "alloc_workqueue" },
	{ 0xed07b4ba, "dma_ops" },
	{ 0xe7a02573, "ida_alloc_range" },
};

MODULE_INFO(depends, "nvmet,memtrack,mlx_compat");


MODULE_INFO(srcversion, "17805E90B7584FFCEA035F2");
