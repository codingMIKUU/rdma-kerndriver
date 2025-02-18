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
	{ 0x2f2c95c4, "flush_work" },
	{ 0x31ca542f, "kmalloc_caches" },
	{ 0x3e0ef07a, "nvmet_wq" },
	{ 0xaf61957e, "nvme_fc_register_localport" },
	{ 0xd12e564, "nvme_fc_register_remoteport" },
	{ 0x754d539c, "strlen" },
	{ 0xb98123d, "nvmet_fc_rcv_ls_req" },
	{ 0x3884f8b8, "nvme_fc_unregister_localport" },
	{ 0xad0413d4, "match_hex" },
	{ 0xf6cb2bfb, "device_destroy" },
	{ 0x44e9a829, "match_token" },
	{ 0x87b8798d, "sg_next" },
	{ 0x85df9b6c, "strsep" },
	{ 0x165b145c, "ex_handler_refcount" },
	{ 0x97651e6c, "vmemmap_base" },
	{ 0xb33d077c, "device_create_with_groups" },
	{ 0x7fa5302a, "nvmet_fc_rcv_fcp_abort" },
	{ 0xb421a321, "pv_ops" },
	{ 0x2d39b0a7, "kstrdup" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x6de13801, "wait_for_completion" },
	{ 0x3812050a, "_raw_spin_unlock_irqrestore" },
	{ 0xc5850110, "printk" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x5223e0e4, "class_unregister" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0x5a921311, "strncmp" },
	{ 0xfca9dc99, "nvme_fc_unregister_remoteport" },
	{ 0x9ef76d99, "nvmet_fc_unregister_targetport" },
	{ 0x4e3567f7, "match_int" },
	{ 0x1048b92a, "nvmet_fc_rcv_fcp_req" },
	{ 0x7cd8d75e, "page_offset_base" },
	{ 0x997c7027, "nvmet_fc_register_targetport" },
	{ 0xa916b694, "strnlen" },
	{ 0xc0e93c71, "put_device" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xb6a43883, "memtrack_inject_error" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xb3a666c0, "kmem_cache_alloc_trace" },
	{ 0xdbf17652, "_raw_spin_lock" },
	{ 0xbb0e18a6, "nvme_fc_rcv_ls_req" },
	{ 0x51760917, "_raw_spin_lock_irqsave" },
	{ 0x83be47e0, "get_device" },
	{ 0x37a0cba, "kfree" },
	{ 0x69acdf38, "memcpy" },
	{ 0xfba7ddd2, "match_u64" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0x3e33ac54, "nvme_fc_rescan_remoteport" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x29361773, "complete" },
	{ 0xcaed4f8, "__class_create" },
};

MODULE_INFO(depends, "nvmet,nvme-fc,nvmet-fc,mlx_compat,memtrack");


MODULE_INFO(srcversion, "6D3632FD6F1C42F3780E52F");
