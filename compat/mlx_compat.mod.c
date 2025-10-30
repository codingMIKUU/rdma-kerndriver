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
	{ 0x2d3385d3, "system_wq" },
	{ 0xb43f9365, "ktime_get" },
	{ 0xde4c1a24, "param_ops_charp" },
	{ 0xc5850110, "printk" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xf4693a66, "call_srcu" },
	{ 0xde09a94d, "xas_find" },
	{ 0xa1691b63, "xas_find_marked" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0xda51119a, "pcie_capability_read_word" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "7D29CA7772F42B30B919A6C");
