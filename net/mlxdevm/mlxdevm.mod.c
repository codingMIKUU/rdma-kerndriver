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
	{ 0x53b954a2, "up_read" },
	{ 0x9fe9bbbc, "genl_register_family" },
	{ 0x984ce9bd, "__nla_parse" },
	{ 0x754d539c, "strlen" },
	{ 0x9da0ba4, "xa_set_mark" },
	{ 0x7878eeba, "genl_unregister_family" },
	{ 0xc7a4fbed, "rtnl_lock" },
	{ 0xb3635b01, "_raw_spin_lock_bh" },
	{ 0xc29957c3, "__x86_indirect_thunk_rcx" },
	{ 0x8fa25c24, "xa_find" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0xb421a321, "pv_ops" },
	{ 0x745a981, "xa_erase" },
	{ 0xc2279809, "skb_trim" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x668b19a1, "down_read" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0xf1db1704, "nla_memcpy" },
	{ 0x131a6146, "xa_clear_mark" },
	{ 0x977f511b, "__mutex_init" },
	{ 0xc5850110, "printk" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0xc0ff12fb, "nla_strdup" },
	{ 0x839e2ef7, "nla_put" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x1765ea1f, "__xa_alloc_cyclic" },
	{ 0x5e93d555, "netlink_unicast" },
	{ 0xce807a25, "up_write" },
	{ 0x57bc19d2, "down_write" },
	{ 0xe22f6d9, "__alloc_skb" },
	{ 0xa916b694, "strnlen" },
	{ 0x49c41a57, "_raw_spin_unlock_bh" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xb6a43883, "memtrack_inject_error" },
	{ 0x8b3b40cb, "kfree_skb" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xb3a666c0, "kmem_cache_alloc_trace" },
	{ 0xdbf17652, "_raw_spin_lock" },
	{ 0xfd1fbfdc, "backport_xa_find_after" },
	{ 0x37a0cba, "kfree" },
	{ 0x69acdf38, "memcpy" },
	{ 0xc7906438, "genlmsg_put" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0x6f18b0e3, "nla_put_64bit" },
	{ 0x7b4da6ff, "__init_rwsem" },
	{ 0x6e720ff2, "rtnl_unlock" },
	{ 0xe914e41e, "strcpy" },
};

MODULE_INFO(depends, "mlx_compat,memtrack");


MODULE_INFO(srcversion, "B352A22A39F2AF53FCDE82C");
