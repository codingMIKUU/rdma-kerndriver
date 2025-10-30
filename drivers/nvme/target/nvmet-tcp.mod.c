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
	{ 0xf3adacf2, "release_sock" },
	{ 0x31ca542f, "kmalloc_caches" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x3e0ef07a, "nvmet_wq" },
	{ 0x840342c6, "sgl_free" },
	{ 0xc306c3a8, "page_frag_alloc" },
	{ 0x97ef76e1, "kernel_sendmsg" },
	{ 0xe613a798, "inet_addr_is_any" },
	{ 0xb50e1c2f, "param_get_int" },
	{ 0xc7a1840e, "llist_add_batch" },
	{ 0xb852606e, "nvmet_req_uninit" },
	{ 0xe73f89c7, "nvmet_ctrl_fatal_error" },
	{ 0xe27d6bbd, "sock_release" },
	{ 0xb3635b01, "_raw_spin_lock_bh" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0x4ae00178, "sock_recvmsg" },
	{ 0x3c12dfe, "cancel_work_sync" },
	{ 0x87b8798d, "sg_next" },
	{ 0x6a12ecfa, "nvmet_sq_init" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0xa4a12f98, "kernel_listen" },
	{ 0x165b145c, "ex_handler_refcount" },
	{ 0x97651e6c, "vmemmap_base" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x8d17bd2a, "__page_frag_cache_drain" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x75d67795, "crypto_ahash_digest" },
	{ 0x1f057e46, "mlx_backport_dependency_symbol" },
	{ 0xa16382b5, "kernel_setsockopt" },
	{ 0x1c82ce7a, "iov_iter_bvec" },
	{ 0xc5850110, "printk" },
	{ 0x53646c2, "lock_sock_nested" },
	{ 0x50153d5e, "is_non_trackable_alloc_func" },
	{ 0x4c9d28b0, "phys_base" },
	{ 0xf848626c, "inet_pton_with_scope" },
	{ 0xffb7c514, "ida_free" },
	{ 0xb020688f, "nvmet_register_transport" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0xf9e627c7, "kernel_sock_shutdown" },
	{ 0xf1969a8e, "__usecs_to_jiffies" },
	{ 0x26a5d65, "kernel_getsockname" },
	{ 0x8ac17111, "kernel_getpeername" },
	{ 0x2276db98, "kstrtoint" },
	{ 0xe7ee6a65, "init_net" },
	{ 0xb3be752, "fput" },
	{ 0x42160169, "flush_workqueue" },
	{ 0x16fccc46, "nvmet_req_init" },
	{ 0x7cd8d75e, "page_offset_base" },
	{ 0xd586a4a, "nvmet_unregister_transport" },
	{ 0x27983cce, "kernel_sendpage" },
	{ 0x3a13f54a, "sgl_alloc" },
	{ 0x49c41a57, "_raw_spin_unlock_bh" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x696547f1, "memtrack_inject_error" },
	{ 0x4f536e99, "memtrack_free" },
	{ 0x88e1d0f0, "page_frag_free" },
	{ 0x1622d999, "is_non_trackable_free_func" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x44b4d14e, "nvmet_sq_destroy" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xef7d6f17, "crypto_destroy_tfm" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xb3a666c0, "kmem_cache_alloc_trace" },
	{ 0x3903f9c3, "sock_alloc_file" },
	{ 0x20ff59cc, "kernel_recvmsg" },
	{ 0xc5b61a34, "kernel_accept" },
	{ 0x8ad29bab, "_raw_write_unlock_bh" },
	{ 0xb320cc0e, "sg_init_one" },
	{ 0x150e3657, "_raw_read_lock_bh" },
	{ 0xad10eb8, "_raw_read_unlock_bh" },
	{ 0x37a0cba, "kfree" },
	{ 0x9f6da3fa, "memtrack_alloc" },
	{ 0x89811968, "sock_create" },
	{ 0xe23d405, "kernel_bind" },
	{ 0xe1ed698d, "_raw_write_lock_bh" },
	{ 0x7a4497db, "kzfree" },
	{ 0xa1f1e265, "nvmet_req_complete" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0xa7d5f92e, "ida_destroy" },
	{ 0xdf9208c0, "alloc_workqueue" },
	{ 0xe7a02573, "ida_alloc_range" },
	{ 0xa57c9bcb, "crypto_alloc_ahash" },
};

MODULE_INFO(depends, "nvmet,mlx_compat,memtrack");


MODULE_INFO(srcversion, "324456554D494A8FE847E63");
