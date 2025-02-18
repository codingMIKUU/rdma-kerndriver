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
	{ 0x7aa1756e, "kvfree" },
	{ 0x27f56ed3, "__auxiliary_driver_register" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0x1b44c663, "current_task" },
	{ 0xc5850110, "printk" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0x82fde53d, "fwctl_register" },
	{ 0x382cb928, "_fwctl_alloc_device" },
	{ 0x599fb41c, "kvmalloc_node" },
	{ 0x2eee3b32, "fwctl_unregister" },
	{ 0xc0e93c71, "put_device" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x9cb986f2, "vmalloc_base" },
	{ 0xb6a43883, "memtrack_inject_error" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0xdf2d7010, "mlx5_cmd_exec" },
	{ 0xfd44fab, "auxiliary_driver_unregister" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xb3a666c0, "kmem_cache_alloc_trace" },
	{ 0xde046cf5, "__dynamic_dev_dbg" },
	{ 0xb6f7a2cf, "mlx5_cmd_do" },
	{ 0x9f6da3fa, "memtrack_alloc" },
};

MODULE_INFO(depends, "auxiliary,mlx_compat,memtrack,fwctl,mlx5_core");


MODULE_INFO(srcversion, "8E7FFA918E1DB6E782658D7");
