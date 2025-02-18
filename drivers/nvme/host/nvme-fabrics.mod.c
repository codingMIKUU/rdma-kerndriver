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
	{ 0xa24f23d8, "__request_module" },
	{ 0x31ca542f, "kmalloc_caches" },
	{ 0x53b954a2, "up_read" },
	{ 0xf6fa2eb9, "single_open" },
	{ 0x754d539c, "strlen" },
	{ 0x3700313a, "single_release" },
	{ 0xad9670f1, "seq_puts" },
	{ 0xacf4d843, "match_strdup" },
	{ 0x4d4d7b79, "blk_mq_map_queues" },
	{ 0x2d2f06d, "memtrack_randomize_mem" },
	{ 0xd981b7ff, "seq_printf" },
	{ 0x837b7b09, "__dynamic_pr_debug" },
	{ 0xf6cb2bfb, "device_destroy" },
	{ 0x44e9a829, "match_token" },
	{ 0x1d07e365, "memdup_user_nul" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0x85df9b6c, "strsep" },
	{ 0x165b145c, "ex_handler_refcount" },
	{ 0xdd64e639, "strscpy" },
	{ 0x4e22ab94, "seq_read" },
	{ 0x2d39b0a7, "kstrdup" },
	{ 0x668b19a1, "down_read" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xb67fec0e, "uuid_parse" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0x9a8377fc, "misc_register" },
	{ 0x62c1325e, "_dev_warn" },
	{ 0xe34eb32f, "__nvme_submit_sync_cmd" },
	{ 0xc5850110, "printk" },
	{ 0x449ad0a7, "memcmp" },
	{ 0x5223e0e4, "class_unregister" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0x5a921311, "strncmp" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x8b58c8f8, "device_create" },
	{ 0x4e3567f7, "match_int" },
	{ 0xce807a25, "up_write" },
	{ 0x885de096, "_dev_err" },
	{ 0x57bc19d2, "down_write" },
	{ 0x69e683de, "uuid_gen" },
	{ 0x75e60613, "key_put" },
	{ 0x4bb4cfba, "module_put" },
	{ 0x589844d9, "_dev_info" },
	{ 0xc0e93c71, "put_device" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xb6a43883, "memtrack_inject_error" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xb3a666c0, "kmem_cache_alloc_trace" },
	{ 0x37a0cba, "kfree" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0x96848186, "scnprintf" },
	{ 0x656e4a6e, "snprintf" },
	{ 0xc60d0620, "__num_online_cpus" },
	{ 0xcaed4f8, "__class_create" },
	{ 0x8f5f598e, "misc_deregister" },
	{ 0x6a71f8d2, "try_module_get" },
};

MODULE_INFO(depends, "memtrack,mlx_compat,nvme-core");


MODULE_INFO(srcversion, "08C4F6CE0DF04DC6BC7870E");
