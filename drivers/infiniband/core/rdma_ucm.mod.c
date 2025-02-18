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
	{ 0x45d6d590, "device_remove_file" },
	{ 0x31ca542f, "kmalloc_caches" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x4a92d3e6, "rdma_get_service_id" },
	{ 0xc1d8cfaf, "__fdget" },
	{ 0xc57c6d80, "unregister_net_sysctl_table" },
	{ 0xd4c14632, "system_unbound_wq" },
	{ 0x3b6f3666, "stream_open" },
	{ 0xa8181adf, "proc_dointvec" },
	{ 0x5b3e282f, "xa_store" },
	{ 0xd992bda2, "rdma_join_multicast" },
	{ 0x2d2f06d, "memtrack_randomize_mem" },
	{ 0x3c12dfe, "cancel_work_sync" },
	{ 0x36c34dc6, "ib_copy_path_rec_to_user" },
	{ 0xef3056b, "rdma_read_gids" },
	{ 0x5338b6a7, "rdma_destroy_id" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0x690989f8, "rdma_connect_ece" },
	{ 0x6d03d716, "ib_copy_ah_attr_to_user" },
	{ 0x29682cb8, "rdma_init_qp_attr" },
	{ 0x165b145c, "ex_handler_refcount" },
	{ 0xb421a321, "pv_ops" },
	{ 0xa0ebc08, "__xa_cmpxchg" },
	{ 0xaaf79faa, "rdma_set_reuseaddr" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0xb44ad4b3, "_copy_to_user" },
	{ 0x6de13801, "wait_for_completion" },
	{ 0x9daf59ac, "rdma_set_ib_path" },
	{ 0xb4c40040, "ib_sa_pack_path" },
	{ 0x9a8377fc, "misc_register" },
	{ 0x1b44c663, "current_task" },
	{ 0xa4db5333, "rdma_accept_ece" },
	{ 0x977f511b, "__mutex_init" },
	{ 0xc5850110, "printk" },
	{ 0x14d6024c, "rdma_listen" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0x4fcb6fac, "rdma_unlock_handler" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x92ae5259, "ib_copy_qp_attr_to_user" },
	{ 0xdf8c1e7e, "rdma_set_afonly" },
	{ 0xe3818e, "rdma_notify" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0xa85a3e6d, "xa_load" },
	{ 0xe7ee6a65, "init_net" },
	{ 0xb3be752, "fput" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0xf40e7a73, "__xa_alloc" },
	{ 0x4e320dbc, "__task_pid_nr_ns" },
	{ 0x597fe5b2, "device_create_file" },
	{ 0xec74867f, "ib_register_client" },
	{ 0xedf4dcb7, "rdma_bind_addr" },
	{ 0xfccdf888, "rdma_resolve_route" },
	{ 0x611ca837, "rdma_create_user_id" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xb6a43883, "memtrack_inject_error" },
	{ 0x1000e51, "schedule" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0xf6f6749e, "rdma_lock_handler" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0xd6636ca6, "rdma_addr_size_in6" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xe783e261, "sysfs_emit" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x78134ea2, "rdma_disconnect" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xb3a666c0, "kmem_cache_alloc_trace" },
	{ 0xdbf17652, "_raw_spin_lock" },
	{ 0x6caa65e7, "rdma_reject" },
	{ 0x86cef180, "rdma_addr_size" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x37a0cba, "kfree" },
	{ 0x69acdf38, "memcpy" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0x2ac2ca8d, "rdma_set_service_type" },
	{ 0xc5a5c78c, "rdma_set_ack_timeout" },
	{ 0xe02c9c92, "__xa_erase" },
	{ 0x728265a7, "rdma_resolve_addr" },
	{ 0x305e5701, "rdma_addr_size_kss" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x9291cd3b, "memdup_user" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x29361773, "complete" },
	{ 0xd21bb37a, "ib_sa_unpack_path" },
	{ 0x1adb353c, "register_net_sysctl" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x8f5f598e, "misc_deregister" },
	{ 0x21498e61, "ib_unregister_client" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x5c9be834, "rdma_leave_multicast" },
};

MODULE_INFO(depends, "rdma_cm,memtrack,ib_uverbs,mlx_compat,ib_core");


MODULE_INFO(srcversion, "9E22D18779CDBFAA5640C26");
