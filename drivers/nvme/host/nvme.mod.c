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
	{ 0x3ce4ca6f, "disable_irq" },
	{ 0x139cee21, "wait_for_completion_io_timeout" },
	{ 0x9170d4f7, "dma_direct_unmap_sg" },
	{ 0x18e60984, "__do_once_start" },
	{ 0x4178f8fd, "__nvme_check_ready" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xf9a482f9, "msleep" },
	{ 0xf888ca21, "sg_init_table" },
	{ 0x1151b3c4, "nvme_get_features" },
	{ 0xdf4ad6bb, "nvme_quiesce_admin_queue" },
	{ 0x4c628eff, "pci_enable_sriov" },
	{ 0x9af93fd1, "pci_free_irq_vectors" },
	{ 0x27947008, "nvme_init_ctrl_finish" },
	{ 0xb50e1c2f, "param_get_int" },
	{ 0x5ab5b891, "param_ops_int" },
	{ 0x9cb583a1, "nvme_init_request" },
	{ 0x49812264, "nvme_wait_reset" },
	{ 0x3f115012, "dma_set_mask" },
	{ 0xccbe3b7a, "nvme_stop_ctrl" },
	{ 0xf38a3177, "pci_disable_device" },
	{ 0x240c7992, "nvme_unfreeze" },
	{ 0xcc8c890e, "nvme_quiesce_io_queues" },
	{ 0x1ffad3a4, "blk_mq_start_request" },
	{ 0xcdf7d68d, "nvme_set_features" },
	{ 0x4d80c86b, "pci_disable_sriov" },
	{ 0x2211a4c0, "blk_op_str" },
	{ 0x4d4d7b79, "blk_mq_map_queues" },
	{ 0x56470118, "__warn_printk" },
	{ 0xa5919b39, "nvme_set_queue_count" },
	{ 0x9034a696, "mempool_destroy" },
	{ 0x87b8798d, "sg_next" },
	{ 0x62962423, "nvme_mark_namespaces_dead" },
	{ 0x785cccc, "blk_mq_tag_to_rq" },
	{ 0x1bfd41ce, "nvme_complete_async_event" },
	{ 0xededc3c3, "param_ops_bool" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0x1f309b21, "dma_free_attrs" },
	{ 0x6d253dca, "dmi_match" },
	{ 0xb5aa7165, "dma_pool_destroy" },
	{ 0xe853152b, "blk_mq_complete_request" },
	{ 0x7a2af7b4, "cpu_number" },
	{ 0x97651e6c, "vmemmap_base" },
	{ 0x2bcbee75, "blk_mq_update_nr_hw_queues" },
	{ 0xed5fe466, "nvme_unquiesce_io_queues" },
	{ 0xb421a321, "pv_ops" },
	{ 0x6d546ce, "dma_set_coherent_mask" },
	{ 0x42635d55, "pm_suspend_global_flags" },
	{ 0xf21017d9, "mutex_trylock" },
	{ 0xc5e4a5d1, "cpumask_next" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x17de3d5, "nr_cpu_ids" },
	{ 0x97b51df7, "pci_set_master" },
	{ 0x4c9a97b0, "pci_alloc_irq_vectors_affinity" },
	{ 0x62c1325e, "_dev_warn" },
	{ 0xfb578fc5, "memset" },
	{ 0x76e84bc4, "pci_enable_pcie_error_reporting" },
	{ 0xb484b72e, "nvme_try_sched_reset" },
	{ 0x9e683f75, "__cpu_possible_mask" },
	{ 0x416d0b22, "nvme_unquiesce_admin_queue" },
	{ 0xd2ddd2a5, "nvme_enable_ctrl" },
	{ 0xe0875eb1, "kstrtobool" },
	{ 0x89793369, "pci_restore_state" },
	{ 0x6a3024b4, "blk_put_queue" },
	{ 0x813cf212, "nvme_io_timeout" },
	{ 0x977f511b, "__mutex_init" },
	{ 0xc5850110, "printk" },
	{ 0x5a5a2271, "__cpu_online_mask" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0xf5c4cebb, "blk_mq_alloc_request" },
	{ 0x2477b5db, "nvme_remove_namespaces" },
	{ 0xa7975388, "pci_read_config_word" },
	{ 0x3db35d93, "dma_direct_map_page" },
	{ 0xae45846e, "dma_alloc_attrs" },
	{ 0xa6643a0a, "pci_device_is_present" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x2f7754a8, "dma_pool_free" },
	{ 0x1fcd648, "blk_execute_rq_nowait" },
	{ 0x55e10d64, "pci_load_saved_state" },
	{ 0xb906d2dc, "pci_request_irq" },
	{ 0x885de096, "_dev_err" },
	{ 0xafdb30cf, "nvme_dev_attrs_group" },
	{ 0x42160169, "flush_workqueue" },
	{ 0xc863967d, "nvme_fail_nonready_command" },
	{ 0x95074f2d, "disk_to_nvme_ns" },
	{ 0x6308439, "nvme_alloc_io_tag_set" },
	{ 0xa7b403b1, "nvme_init_ctrl" },
	{ 0x2330e2e4, "pci_select_bars" },
	{ 0x87d16318, "dma_direct_unmap_page" },
	{ 0x589844d9, "_dev_info" },
	{ 0xe098d730, "nvme_cancel_tagset" },
	{ 0xd3822959, "nvme_change_ctrl_state" },
	{ 0x62367f42, "blk_mq_free_request" },
	{ 0x54085d0d, "__tracepoint_nvme_sq" },
	{ 0xc3762aec, "mempool_alloc" },
	{ 0x93a219c, "ioremap_nocache" },
	{ 0x9c122bcf, "mempool_create_node" },
	{ 0x38b0c2db, "pci_free_irq" },
	{ 0xc0e93c71, "put_device" },
	{ 0x87085c3a, "dma_max_mapping_size" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xc945776f, "nvme_sync_queues" },
	{ 0x9363fffa, "param_get_uint" },
	{ 0xffe3addd, "nvme_cleanup_cmd" },
	{ 0xb6a43883, "memtrack_inject_error" },
	{ 0x8e892ac8, "nvme_remove_io_tag_set" },
	{ 0x1d24c881, "___ratelimit" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0xfea95117, "nvme_wait_freeze" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0xb4f1e323, "blk_mq_pci_map_queues" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0x6a037cf1, "mempool_kfree" },
	{ 0xc9c7566, "blk_rq_map_sg" },
	{ 0x678b96ec, "dma_pool_alloc" },
	{ 0xe783e261, "sysfs_emit" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x3b67ff49, "pci_unregister_driver" },
	{ 0xa897e3e7, "mempool_free" },
	{ 0xdbf17652, "_raw_spin_lock" },
	{ 0x5bc962c8, "nvme_disable_ctrl" },
	{ 0xb19a5453, "__per_cpu_offset" },
	{ 0x83be47e0, "get_device" },
	{ 0x1c56b625, "pci_irq_vector" },
	{ 0x64b62862, "nvme_wq" },
	{ 0xd35a6d31, "mempool_kmalloc" },
	{ 0xfda18761, "pci_disable_pcie_error_reporting" },
	{ 0xfcec0987, "enable_irq" },
	{ 0x37a0cba, "kfree" },
	{ 0x69ad2f20, "kstrtouint" },
	{ 0x77145184, "dma_direct_map_sg" },
	{ 0xf4c7fc7a, "nvme_submit_sync_cmd" },
	{ 0x6e97c5c8, "param_set_uint" },
	{ 0xdf726648, "nvme_alloc_admin_tag_set" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0xedc03953, "iounmap" },
	{ 0xc6de2696, "pcibios_resource_to_bus" },
	{ 0xac296b6b, "nvme_start_ctrl" },
	{ 0xfbfddd92, "__pci_register_driver" },
	{ 0x70282e21, "nvme_start_freeze" },
	{ 0x96848186, "scnprintf" },
	{ 0xbe27e3d2, "nvme_setup_cmd" },
	{ 0xd45434ee, "admin_timeout" },
	{ 0x63c4d61f, "__bitmap_weight" },
	{ 0xece509fd, "sysfs_add_file_to_group" },
	{ 0x1ba59527, "__kmalloc_node" },
	{ 0x17e79915, "pci_vfs_assigned" },
	{ 0x29361773, "complete" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x6cff6037, "nvme_cancel_admin_tagset" },
	{ 0xee009719, "pci_enable_device_mem" },
	{ 0xf63b102e, "nvme_wait_freeze_timeout" },
	{ 0x7dc91072, "pci_release_selected_regions" },
	{ 0xac2c36dc, "pci_request_selected_regions" },
	{ 0x4f447205, "nvme_complete_rq" },
	{ 0x999f25e3, "param_ops_uint" },
	{ 0xfbd70d6e, "dma_pool_create" },
	{ 0xe65535bf, "nvme_uninit_ctrl" },
	{ 0xed07b4ba, "dma_ops" },
	{ 0x39b0b193, "pcie_aspm_enabled" },
	{ 0x784ee34b, "pci_save_state" },
	{ 0x4198ca95, "__do_once_done" },
	{ 0x81183f0, "nvme_remove_admin_tag_set" },
};

MODULE_INFO(depends, "nvme-core,mlx_compat,memtrack");

MODULE_ALIAS("pci:v00008086d00000953sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00000A53sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00000A54sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00000A55sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d0000F1A5sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d0000F1A6sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d00005845sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v0000126Fd00002262sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v0000126Fd00002263sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001BB1d00000100sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001C58d00000003sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001C58d00000023sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001C5Fd00000540sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v0000144Dd0000A821sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v0000144Dd0000A822sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000015B7d00005008sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001987d00005012sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001987d00005016sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001987d00005019sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001987d00005021sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001B4Bd00001092sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001CC1d000033F8sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010ECd00005762sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010ECd00005763sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001CC1d00008201sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001344d00005407sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001344d00006001sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001C5Cd00001504sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001C5Cd0000174Asv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001C5Cd00001D59sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000015B7d00002001sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001D97d00002263sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v0000144Dd0000A80Bsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v0000144Dd0000A809sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v0000144Dd0000A802sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001CC4d00006303sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001CC4d00006302sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00002646d00002262sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00002646d00002263sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00002646d00005013sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00002646d00005018sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00002646d00005016sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00002646d0000501Asv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00002646d0000501Bsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00002646d0000501Esv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001F40d00001202sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001F40d00005236sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001E4Bd00001001sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001E4Bd00001002sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001E4Bd00001202sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001E4Bd00001602sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001CC1d00005350sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001DBEd00005236sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001E49d00000021sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001E49d00000041sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v0000C0A9d0000540Asv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001D97d00002263sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001D97d00001D97sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001D97d00002269sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010ECd00005763sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001E4Bd00001602sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010ECd00005765sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001D0Fd00000061sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001D0Fd00000065sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001D0Fd00008061sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001D0Fd0000CD00sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001D0Fd0000CD01sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001D0Fd0000CD02sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v0000106Bd00002001sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v0000106Bd00002003sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v0000106Bd00002005sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v*d*sv*sd*bc01sc08i02*");

MODULE_INFO(srcversion, "5037A37CE35C6BB656C9448");
