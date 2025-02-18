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
	{ 0xeb233a45, "__kmalloc" },
	{ 0x1ed8b599, "__x86_indirect_thunk_r8" },
	{ 0x7381287f, "trace_handle_return" },
	{ 0x39c1a57f, "ib_create_ah_from_wc" },
	{ 0xf9909395, "ib_modify_mad" },
	{ 0xe25ee9d3, "_raw_write_lock_irqsave" },
	{ 0x19f462ab, "kfree_call_rcu" },
	{ 0x79aa04a2, "get_random_bytes" },
	{ 0x7f2e033, "ib_register_mad_agent" },
	{ 0x33e0dd1, "ib_free_recv_mad" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0x5b3e282f, "xa_store" },
	{ 0x2d2f06d, "memtrack_randomize_mem" },
	{ 0x56470118, "__warn_printk" },
	{ 0x66b4cc41, "kmemdup" },
	{ 0xc29957c3, "__x86_indirect_thunk_rcx" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xec885911, "rdma_find_gid" },
	{ 0xeadb83a, "trace_event_buffer_reserve" },
	{ 0x165b145c, "ex_handler_refcount" },
	{ 0x7a2af7b4, "cpu_number" },
	{ 0xcee8585a, "rdma_move_ah_attr" },
	{ 0x50bd11ef, "bpf_trace_run3" },
	{ 0xb421a321, "pv_ops" },
	{ 0x745a981, "xa_erase" },
	{ 0xc020b134, "ib_port_unregister_client_groups" },
	{ 0x1c1b9f8e, "_raw_write_unlock_irqrestore" },
	{ 0x76e1b19, "ib_free_send_mad" },
	{ 0xd50dbf35, "rdma_destroy_ah_user" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x52b76ecd, "trace_define_field" },
	{ 0x62c1325e, "_dev_warn" },
	{ 0x3dad9978, "cancel_delayed_work" },
	{ 0xfaa4afa8, "bpf_trace_run1" },
	{ 0x3812050a, "_raw_spin_unlock_irqrestore" },
	{ 0xc5850110, "printk" },
	{ 0x5a5a2271, "__cpu_online_mask" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0x4d9b652b, "rb_erase" },
	{ 0x4e7f474a, "trace_event_reg" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0x1765ea1f, "__xa_alloc_cyclic" },
	{ 0x5cb381e4, "rdma_put_gid_attr" },
	{ 0xa85a3e6d, "xa_load" },
	{ 0x482fa476, "ib_get_cached_pkey" },
	{ 0xa011149d, "ib_set_client_data" },
	{ 0x4d2df799, "rdma_destroy_ah_attr" },
	{ 0xbfafa85, "perf_trace_run_bpf_submit" },
	{ 0xad5f0017, "perf_trace_buf_alloc" },
	{ 0x42160169, "flush_workqueue" },
	{ 0x38f91238, "ib_create_send_mad" },
	{ 0xb5401bb9, "ib_post_send_mad" },
	{ 0x2fb78152, "ib_unregister_mad_agent" },
	{ 0xec74867f, "ib_register_client" },
	{ 0x736b5662, "_raw_read_lock_irqsave" },
	{ 0xb601be4c, "__x86_indirect_thunk_rdx" },
	{ 0xac81d670, "trace_event_ignore_this_pid" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xb6a43883, "memtrack_inject_error" },
	{ 0xb8b9f817, "kmalloc_order_trace" },
	{ 0x1d24c881, "___ratelimit" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x47941711, "_raw_spin_lock_irq" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0x6df6a215, "ib_find_cached_pkey" },
	{ 0xa16c8613, "_raw_read_unlock_irqrestore" },
	{ 0x898a8a5d, "ib_init_ah_attr_from_wc" },
	{ 0xbe6abb19, "rdma_create_ah" },
	{ 0x967137db, "trace_event_buffer_commit" },
	{ 0xe783e261, "sysfs_emit" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xb3a666c0, "kmem_cache_alloc_trace" },
	{ 0xdbf17652, "_raw_spin_lock" },
	{ 0x51760917, "_raw_spin_lock_irqsave" },
	{ 0xa5526619, "rb_insert_color" },
	{ 0xedd8801c, "event_triggers_call" },
	{ 0xe90b9eed, "bpf_trace_run2" },
	{ 0xbbee4e14, "ib_modify_port" },
	{ 0xa201811b, "ib_port_register_client_groups" },
	{ 0xcc767f39, "rdma_query_gid" },
	{ 0x37a0cba, "kfree" },
	{ 0xcc5c2df4, "trace_print_symbols_seq" },
	{ 0x5afcea1e, "trace_event_raw_init" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0x53569707, "this_cpu_off" },
	{ 0xe101a7b4, "trace_raw_output_prep" },
	{ 0x29361773, "complete" },
	{ 0x525d0aa3, "trace_seq_printf" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0x4d1ff60a, "wait_for_completion_timeout" },
	{ 0x95686ff1, "ib_init_ah_attr_from_path" },
	{ 0xdf9208c0, "alloc_workqueue" },
	{ 0x21498e61, "ib_unregister_client" },
};

MODULE_INFO(depends, "ib_core,memtrack,mlx_compat");


MODULE_INFO(srcversion, "9561E3CE9B6FB68C623F825");
