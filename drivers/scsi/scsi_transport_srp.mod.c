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
	{ 0xb771b3f3, "transport_class_register" },
	{ 0x31ca542f, "kmalloc_caches" },
	{ 0xa8dfe9da, "dev_printk" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0x837b7b09, "__dynamic_pr_debug" },
	{ 0x7212f5fe, "transport_destroy_device" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0xb16c1e41, "attribute_container_unregister" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x131db64a, "system_long_wq" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0xca554822, "scsi_is_host_device" },
	{ 0x3dad9978, "cancel_delayed_work" },
	{ 0x2f210362, "device_del" },
	{ 0x2db3d320, "mutex_lock_interruptible" },
	{ 0xfc807d2a, "transport_add_device" },
	{ 0x977f511b, "__mutex_init" },
	{ 0xc5850110, "printk" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0x20dc5d6b, "transport_configure_device" },
	{ 0x2a72b036, "attribute_container_register" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x2276db98, "kstrtoint" },
	{ 0x4c2bd6f7, "device_add" },
	{ 0x6b2c3409, "transport_class_unregister" },
	{ 0xf32116d5, "scsi_target_unblock" },
	{ 0xc0e93c71, "put_device" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x696547f1, "memtrack_inject_error" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xdbece28e, "transport_setup_device" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xb3a666c0, "kmem_cache_alloc_trace" },
	{ 0x83be47e0, "get_device" },
	{ 0x4ba3b3d5, "__scsi_iterate_devices" },
	{ 0xa87b1e07, "device_for_each_child" },
	{ 0x37a0cba, "kfree" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0xd8e54085, "device_initialize" },
	{ 0x37ed5381, "transport_remove_device" },
	{ 0x56f63f7b, "scsi_target_block" },
	{ 0xae845f61, "dev_set_name" },
};

MODULE_INFO(depends, "mlx_compat,memtrack");


MODULE_INFO(srcversion, "DF1E43BF6F412F1C016A953");
