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
	{ 0xdc405352, "__nvme_check_ready" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x3e0ef07a, "nvmet_wq" },
	{ 0x685230e0, "nvme_quiesce_admin_queue" },
	{ 0x1bee4974, "sg_alloc_table_chained" },
	{ 0xfefd4731, "nvme_init_ctrl_finish" },
	{ 0x3d736483, "nvme_stop_ctrl" },
	{ 0x563afbff, "nvme_quiesce_io_queues" },
	{ 0x1ffad3a4, "blk_mq_start_request" },
	{ 0x19b4ab91, "nvmf_register_transport" },
	{ 0x18c8183d, "nvme_set_queue_count" },
	{ 0x785cccc, "blk_mq_tag_to_rq" },
	{ 0xbecf5eda, "nvme_complete_async_event" },
	{ 0x6a12ecfa, "nvmet_sq_init" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0xe853152b, "blk_mq_complete_request" },
	{ 0x8df69372, "nvmf_reg_write32" },
	{ 0x2bcbee75, "blk_mq_update_nr_hw_queues" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0x62c1325e, "_dev_warn" },
	{ 0x75c1d6b4, "nvme_unquiesce_admin_queue" },
	{ 0xe3ab8486, "nvme_enable_ctrl" },
	{ 0xc5850110, "printk" },
	{ 0x251907f4, "nvmf_connect_admin_queue" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0xb11a4ca1, "nvmf_reg_read64" },
	{ 0xb020688f, "nvmet_register_transport" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x885de096, "_dev_err" },
	{ 0x42160169, "flush_workqueue" },
	{ 0xf94cd7b4, "nvme_fail_nonready_command" },
	{ 0x16fccc46, "nvmet_req_init" },
	{ 0x571b5bce, "nvme_alloc_io_tag_set" },
	{ 0xd58bbbcb, "nvme_delete_wq" },
	{ 0x45af4f1, "nvme_init_ctrl" },
	{ 0x589844d9, "_dev_info" },
	{ 0xb290f589, "nvme_cancel_tagset" },
	{ 0x7ec83fa9, "nvme_change_ctrl_state" },
	{ 0xd586a4a, "nvmet_unregister_transport" },
	{ 0xc0e93c71, "put_device" },
	{ 0x63716ea3, "nvmf_unregister_transport" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x3ebbe88b, "nvme_cleanup_cmd" },
	{ 0x696547f1, "memtrack_inject_error" },
	{ 0xf96567c4, "nvme_remove_io_tag_set" },
	{ 0xb8b9f817, "kmalloc_order_trace" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x44b4d14e, "nvmet_sq_destroy" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xf17e667c, "nvmf_get_address" },
	{ 0xc9c7566, "blk_rq_map_sg" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xd0250c58, "nvme_disable_ctrl" },
	{ 0x55cce6d5, "nvme_delete_ctrl" },
	{ 0xca208232, "nvmf_reg_read32" },
	{ 0x274dd1a3, "sg_free_table_chained" },
	{ 0x37a0cba, "kfree" },
	{ 0xcb5f874f, "nvmf_connect_io_queue" },
	{ 0x378676fe, "nvme_alloc_admin_tag_set" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0x755a8cae, "nvme_start_ctrl" },
	{ 0x46a7a71f, "nvme_setup_cmd" },
	{ 0xfcb2b86b, "nvmf_free_options" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x8588ea89, "nvme_cancel_admin_tagset" },
	{ 0xc60d0620, "__num_online_cpus" },
	{ 0xc1537082, "nvme_complete_rq" },
	{ 0x116dcc1b, "nvme_uninit_ctrl" },
	{ 0x1d495350, "nvme_remove_admin_tag_set" },
};

MODULE_INFO(depends, "nvme-core,nvmet,nvme-fabrics,mlx_compat,memtrack");


MODULE_INFO(srcversion, "51B7BC47A7763EBCDDF32BF");
