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
	{ 0xeb233a45, "__kmalloc" },
	{ 0xb5739c63, "cdev_init" },
	{ 0x53b954a2, "up_read" },
	{ 0x7aa1756e, "kvfree" },
	{ 0x754d539c, "strlen" },
	{ 0x6729d3df, "__get_user_4" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x668b19a1, "down_read" },
	{ 0xfb384d37, "kasprintf" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0xb44ad4b3, "_copy_to_user" },
	{ 0x62c1325e, "_dev_warn" },
	{ 0xfb578fc5, "memset" },
	{ 0x1b44c663, "current_task" },
	{ 0x977f511b, "__mutex_init" },
	{ 0xc5850110, "printk" },
	{ 0x5223e0e4, "class_unregister" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0xffb7c514, "ida_free" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0xce807a25, "up_write" },
	{ 0x6e200dfa, "__class_register" },
	{ 0x57bc19d2, "down_write" },
	{ 0x599fb41c, "kvmalloc_node" },
	{ 0xc6cbbc89, "capable" },
	{ 0x76d451c4, "add_taint" },
	{ 0xc0e93c71, "put_device" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x9cb986f2, "vmalloc_base" },
	{ 0x696547f1, "memtrack_inject_error" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x98e8cd7f, "cdev_device_add" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x83be47e0, "get_device" },
	{ 0x37a0cba, "kfree" },
	{ 0x2db3bc61, "check_zeroed_user" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0xd8e54085, "device_initialize" },
	{ 0x8848b4a4, "cdev_device_del" },
	{ 0xae845f61, "dev_set_name" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x7b4da6ff, "__init_rwsem" },
	{ 0x7aec9089, "clear_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xe7a02573, "ida_alloc_range" },
};

MODULE_INFO(depends, "mlx_compat,memtrack");


MODULE_INFO(srcversion, "7D087F991185EDA2E9BC08E");
