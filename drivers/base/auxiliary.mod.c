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
	{ 0xba1d6f14, "bus_register" },
	{ 0x31ca542f, "kmalloc_caches" },
	{ 0x4f6b0676, "driver_register" },
	{ 0x754d539c, "strlen" },
	{ 0x5b3e282f, "xa_store" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0xb421a321, "pv_ops" },
	{ 0x745a981, "xa_erase" },
	{ 0x296cb509, "__xa_insert" },
	{ 0xfb384d37, "kasprintf" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0x62c1325e, "_dev_warn" },
	{ 0x977f511b, "__mutex_init" },
	{ 0xc5850110, "printk" },
	{ 0xc41d1af4, "sysfs_remove_file_from_group" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0xb4a319a9, "driver_unregister" },
	{ 0xe7b00dfb, "__x86_indirect_thunk_r13" },
	{ 0x5a921311, "strncmp" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0xa85a3e6d, "xa_load" },
	{ 0x4c2bd6f7, "device_add" },
	{ 0x94e6746c, "bus_find_device" },
	{ 0x885de096, "_dev_err" },
	{ 0x82bc87f6, "bus_unregister" },
	{ 0xae7f3308, "pm_generic_suspend" },
	{ 0xf9758623, "pm_generic_runtime_suspend" },
	{ 0x717d3462, "dev_pm_domain_detach" },
	{ 0x9f984513, "strrchr" },
	{ 0x696547f1, "memtrack_inject_error" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0x772b4e2, "pm_generic_resume" },
	{ 0x6572ba96, "pm_generic_runtime_resume" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xb3a666c0, "kmem_cache_alloc_trace" },
	{ 0xdbf17652, "_raw_spin_lock" },
	{ 0x4217576, "devm_device_add_group" },
	{ 0x998bb42e, "dev_pm_domain_attach" },
	{ 0x37a0cba, "kfree" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0xd8e54085, "device_initialize" },
	{ 0xece509fd, "sysfs_add_file_to_group" },
	{ 0x656e4a6e, "snprintf" },
	{ 0xae845f61, "dev_set_name" },
	{ 0x719e0e44, "add_uevent_var" },
};

MODULE_INFO(depends, "mlx_compat,memtrack");


MODULE_INFO(srcversion, "7CE250BFD8C8A1B83A81007");
