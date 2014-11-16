#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x80567cab, "module_layout" },
	{ 0x3d0db4d5, "free_netdev" },
	{ 0xb87e1e7f, "unregister_netdev" },
	{ 0x1c7708eb, "register_netdev" },
	{ 0x384c71fc, "alloc_netdev_mqs" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xc9a49e2c, "__register_chrdev" },
	{ 0xe174aa7, "__init_waitqueue_head" },
	{ 0x6e712077, "kmem_cache_alloc_trace" },
	{ 0xaa5b0f7, "kmalloc_caches" },
	{ 0xe914e41e, "strcpy" },
	{ 0x7d11c268, "jiffies" },
	{ 0x6710e7e5, "consume_skb" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x7cb24d2d, "netif_rx" },
	{ 0x91f322c2, "eth_type_trans" },
	{ 0x1ed9e8f3, "skb_put" },
	{ 0xc90141ec, "dev_alloc_skb" },
	{ 0x16305289, "warn_slowpath_null" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0xb85f3bbe, "pv_lock_ops" },
	{ 0x6443d74d, "_raw_spin_lock" },
	{ 0xb89e62ec, "remove_wait_queue" },
	{ 0x4292364c, "schedule" },
	{ 0xda5661a3, "add_wait_queue" },
	{ 0xffd5a395, "default_wake_function" },
	{ 0xfcf580f6, "current_task" },
	{ 0xf09c7f68, "__wake_up" },
	{ 0x2bc95bd4, "memset" },
	{ 0x50eedeb8, "printk" },
	{ 0x148c3f96, "__netif_schedule" },
	{ 0x88941a06, "_raw_spin_unlock_irqrestore" },
	{ 0x37a0cba, "kfree" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x587c70d8, "_raw_spin_lock_irqsave" },
	{ 0x2e60bace, "memcpy" },
	{ 0xd335c45d, "skb_push" },
	{ 0x8e6d2041, "ether_setup" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "9DAA338414894F0FC857EDB");
