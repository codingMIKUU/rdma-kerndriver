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
	{ 0x4178f8fd, "__nvme_check_ready" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x3e0ef07a, "nvmet_wq" },
	{ 0xdf4ad6bb, "nvme_quiesce_admin_queue" },
	{ 0x1bee4974, "sg_alloc_table_chained" },
	{ 0x27947008, "nvme_init_ctrl_finish" },
	{ 0xccbe3b7a, "nvme_stop_ctrl" },
	{ 0xcc8c890e, "nvme_quiesce_io_queues" },
	{ 0x1ffad3a4, "blk_mq_start_request" },
	{ 0x3a513fc9, "nvmf_register_transport" },
	{ 0xa5919b39, "nvme_set_queue_count" },
	{ 0x785cccc, "blk_mq_tag_to_rq" },
	{ 0x1bfd41ce, "nvme_complete_async_event" },
	{ 0xc8a5ba06, "nvmet_sq_init" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0xe853152b, "blk_mq_complete_request" },
	{ 0x8d49784e, "nvmf_reg_write32" },
	{ 0x2bcbee75, "blk_mq_update_nr_hw_queues" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0x62c1325e, "_dev_warn" },
	{ 0x416d0b22, "nvme_unquiesce_admin_queue" },
	{ 0xd2ddd2a5, "nvme_enable_ctrl" },
	{ 0xc5850110, "printk" },
	{ 0x31ca5459, "nvmf_connect_admin_queue" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0xce754cff, "nvmf_reg_read64" },
	{ 0x51cdf94b, "nvmet_register_transport" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x885de096, "_dev_err" },
	{ 0x42160169, "flush_workqueue" },
	{ 0xc863967d, "nvme_fail_nonready_command" },
	{ 0x1ca975f9, "nvmet_req_init" },
	{ 0x6308439, "nvme_alloc_io_tag_set" },
	{ 0xd58bbbcb, "nvme_delete_wq" },
	{ 0xa7b403b1, "nvme_init_ctrl" },
	{ 0x589844d9, "_dev_info" },
	{ 0xe098d730, "nvme_cancel_tagset" },
	{ 0xd3822959, "nvme_change_ctrl_state" },
	{ 0xa005c413, "nvmet_unregister_transport" },
	{ 0xc0e93c71, "put_device" },
	{ 0xab176b0d, "nvmf_unregister_transport" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xffe3addd, "nvme_cleanup_cmd" },
	{ 0xb6a43883, "memtrack_inject_error" },
	{ 0x8e892ac8, "nvme_remove_io_tag_set" },
	{ 0xb8b9f817, "kmalloc_order_trace" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x67235b02, "nvmet_sq_destroy" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xcd7c66ef, "nvmf_get_address" },
	{ 0xc9c7566, "blk_rq_map_sg" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x5bc962c8, "nvme_disable_ctrl" },
	{ 0xc15fc28a, "nvme_delete_ctrl" },
	{ 0x28173133, "nvmf_reg_read32" },
	{ 0x274dd1a3, "sg_free_table_chained" },
	{ 0x37a0cba, "kfree" },
	{ 0x49a70397, "nvmf_connect_io_queue" },
	{ 0xdf726648, "nvme_alloc_admin_tag_set" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0xac296b6b, "nvme_start_ctrl" },
	{ 0xbe27e3d2, "nvme_setup_cmd" },
	{ 0x7e786900, "nvmf_free_options" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x6cff6037, "nvme_cancel_admin_tagset" },
	{ 0xc60d0620, "__num_online_cpus" },
	{ 0x4f447205, "nvme_complete_rq" },
	{ 0xe65535bf, "nvme_uninit_ctrl" },
	{ 0x81183f0, "nvme_remove_admin_tag_set" },
};

MODULE_INFO(depends, "nvme-core,nvmet,nvme-fabrics,mlx_compat,memtrack");


MODULE_INFO(srcversion, "51B7BC47A7763EBCDDF32BF");
