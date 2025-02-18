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
	{ 0x31ca542f, "kmalloc_caches" },
	{ 0x174f52a7, "rdma_nl_register" },
	{ 0x984ce9bd, "__nla_parse" },
	{ 0x754d539c, "strlen" },
	{ 0xc57c6d80, "unregister_net_sysctl_table" },
	{ 0xa8181adf, "proc_dointvec" },
	{ 0x2d2f06d, "memtrack_randomize_mem" },
	{ 0x837b7b09, "__dynamic_pr_debug" },
	{ 0x7634c1cf, "rdma_nl_unicast_wait" },
	{ 0x66b4cc41, "kmemdup" },
	{ 0xb3f0d4a3, "__dev_kfree_skb_any" },
	{ 0x165b145c, "ex_handler_refcount" },
	{ 0x616dd8eb, "ib_modify_qp" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x7fbaf1db, "__netdev_alloc_skb" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x3812050a, "_raw_spin_unlock_irqrestore" },
	{ 0xc5850110, "printk" },
	{ 0x7379379e, "ibnl_put_attr" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0x6626afca, "down" },
	{ 0xe7ee6a65, "init_net" },
	{ 0x1628451b, "rdma_nl_multicast" },
	{ 0x42160169, "flush_workqueue" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x75a729a0, "rdma_nl_unregister" },
	{ 0x6c43ad14, "rdma_nl_unicast" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xb6a43883, "memtrack_inject_error" },
	{ 0x1000e51, "schedule" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xb3a666c0, "kmem_cache_alloc_trace" },
	{ 0x51760917, "_raw_spin_lock_irqsave" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x37a0cba, "kfree" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0xcf2a6966, "up" },
	{ 0x92540fbf, "finish_wait" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x1fc7a59b, "__nla_validate" },
	{ 0x55940b88, "consume_skb" },
	{ 0x1adb353c, "register_net_sysctl" },
	{ 0x4e67819e, "ibnl_put_msg" },
	{ 0xdf9208c0, "alloc_workqueue" },
	{ 0x9682235, "down_timeout" },
	{ 0xd542439, "__ipv6_addr_type" },
};

MODULE_INFO(depends, "ib_core,memtrack,mlx_compat");


MODULE_INFO(srcversion, "B3889EC82A965906E3F0C19");
