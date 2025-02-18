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
	{ 0xb5739c63, "cdev_init" },
	{ 0x1ed8b599, "__x86_indirect_thunk_r8" },
	{ 0x7381287f, "trace_handle_return" },
	{ 0x7aa1756e, "kvfree" },
	{ 0x754d539c, "strlen" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0x7f2e033, "ib_register_mad_agent" },
	{ 0x3b6f3666, "stream_open" },
	{ 0x33e0dd1, "ib_free_recv_mad" },
	{ 0x81b395b3, "down_interruptible" },
	{ 0x56470118, "__warn_printk" },
	{ 0x6729d3df, "__get_user_4" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0xeadb83a, "trace_event_buffer_reserve" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x165b145c, "ex_handler_refcount" },
	{ 0x7a2af7b4, "cpu_number" },
	{ 0x43259b3f, "_dev_notice" },
	{ 0x50bd11ef, "bpf_trace_run3" },
	{ 0xb421a321, "pv_ops" },
	{ 0xe97c5a4f, "nonseekable_open" },
	{ 0x6c8ac0bd, "ib_get_rmpp_segment" },
	{ 0xfb384d37, "kasprintf" },
	{ 0x76e1b19, "ib_free_send_mad" },
	{ 0xd50dbf35, "rdma_destroy_ah_user" },
	{ 0xc6841f1d, "rdma_create_user_ah" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0x613b1e2e, "ib_is_mad_class_rmpp" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0xb44ad4b3, "_copy_to_user" },
	{ 0x52b76ecd, "trace_define_field" },
	{ 0x62c1325e, "_dev_warn" },
	{ 0x1b44c663, "current_task" },
	{ 0xfc7e2596, "down_trylock" },
	{ 0x977f511b, "__mutex_init" },
	{ 0xc5850110, "printk" },
	{ 0x5a5a2271, "__cpu_online_mask" },
	{ 0x5223e0e4, "class_unregister" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0x6bc06296, "ib_mad_kernel_rmpp_agent" },
	{ 0xffb7c514, "ida_free" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x4e7f474a, "trace_event_reg" },
	{ 0x5a921311, "strncmp" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x6e200dfa, "__class_register" },
	{ 0xa011149d, "ib_set_client_data" },
	{ 0x4d2df799, "rdma_destroy_ah_attr" },
	{ 0xbfafa85, "perf_trace_run_bpf_submit" },
	{ 0xad5f0017, "perf_trace_buf_alloc" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x38f91238, "ib_create_send_mad" },
	{ 0xb5401bb9, "ib_post_send_mad" },
	{ 0x4e155af0, "ib_response_mad" },
	{ 0x2fb78152, "ib_unregister_mad_agent" },
	{ 0x599fb41c, "kvmalloc_node" },
	{ 0xec74867f, "ib_register_client" },
	{ 0xb2fd5ceb, "__put_user_4" },
	{ 0xac81d670, "trace_event_ignore_this_pid" },
	{ 0xc0e93c71, "put_device" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x9cb986f2, "vmalloc_base" },
	{ 0xb6a43883, "memtrack_inject_error" },
	{ 0x1000e51, "schedule" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x98e8cd7f, "cdev_device_add" },
	{ 0x47941711, "_raw_spin_lock_irq" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0x898a8a5d, "ib_init_ah_attr_from_wc" },
	{ 0x967137db, "trace_event_buffer_commit" },
	{ 0xe783e261, "sysfs_emit" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xb3a666c0, "kmem_cache_alloc_trace" },
	{ 0xe1f0b774, "rdma_dev_access_netns" },
	{ 0xedd8801c, "event_triggers_call" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0xbbee4e14, "ib_modify_port" },
	{ 0x37a0cba, "kfree" },
	{ 0x5afcea1e, "trace_event_raw_init" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0xd8e54085, "device_initialize" },
	{ 0xcf2a6966, "up" },
	{ 0x53569707, "this_cpu_off" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x8848b4a4, "cdev_device_del" },
	{ 0xe101a7b4, "trace_raw_output_prep" },
	{ 0x525d0aa3, "trace_seq_printf" },
	{ 0xae845f61, "dev_set_name" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x787ccc4c, "ib_get_mad_data_offset" },
	{ 0x21498e61, "ib_unregister_client" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xe7a02573, "ida_alloc_range" },
};

MODULE_INFO(depends, "ib_core,mlx_compat,memtrack");


MODULE_INFO(srcversion, "F54797E71CA8512A9CCE553");
