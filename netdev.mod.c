#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
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
__used __section("__versions") = {
	{ 0x19fdd0c3, "module_layout" },
	{ 0x5f3af44c, "kmalloc_caches" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x69e052b6, "param_ops_int" },
	{ 0x1e5c8b3f, "napi_disable" },
	{ 0xe3501dc2, "napi_schedule_prep" },
	{ 0x2124474, "ip_send_check" },
	{ 0x837b7b09, "__dynamic_pr_debug" },
	{ 0xfd9d9118, "pv_ops" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x3c2479a4, "__netdev_alloc_skb" },
	{ 0xe6324690, "netif_rx" },
	{ 0xaa65f1f2, "netif_tx_wake_queue" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0xc5850110, "printk" },
	{ 0x81071c88, "free_netdev" },
	{ 0x749f89bb, "register_netdev" },
	{ 0x75e095cf, "netif_receive_skb" },
	{ 0x889d56b8, "skb_push" },
	{ 0xae149bb5, "netif_napi_add" },
	{ 0x167c5967, "print_hex_dump" },
	{ 0xe2aef0f, "__napi_schedule" },
	{ 0x378acf3b, "alloc_netdev_mqs" },
	{ 0x5c17432a, "napi_complete_done" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0x7853ce0d, "eth_type_trans" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xc39b1f72, "ether_setup" },
	{ 0xd1ff3d1f, "kmem_cache_alloc_trace" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0x37a0cba, "kfree" },
	{ 0x69acdf38, "memcpy" },
	{ 0x6128b5fc, "__printk_ratelimit" },
	{ 0x85f1e081, "unregister_netdev" },
	{ 0xc45168fe, "consume_skb" },
	{ 0x2fd2a6f2, "skb_put" },
	{ 0x342e88e3, "__skb_pad" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "9C034407F7DF2B6EBA91CF2");
