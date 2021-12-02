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
	{ 0xeeab4c1e, "module_layout" },
	{ 0x6aa9e858, "kmalloc_caches" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x93c68c79, "param_ops_int" },
	{ 0x82345646, "napi_disable" },
	{ 0xff20050c, "napi_schedule_prep" },
	{ 0x2124474, "ip_send_check" },
	{ 0x837b7b09, "__dynamic_pr_debug" },
	{ 0x1a81ff9c, "pv_ops" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x9696f369, "__netdev_alloc_skb" },
	{ 0x6ee414e5, "netif_rx" },
	{ 0xa3d1e4a, "netif_tx_wake_queue" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0xc5850110, "printk" },
	{ 0xe03babfd, "free_netdev" },
	{ 0x9d0c9561, "register_netdev" },
	{ 0xdaeda105, "netif_receive_skb" },
	{ 0xff029bf0, "skb_push" },
	{ 0x7351e315, "netif_napi_add" },
	{ 0x167c5967, "print_hex_dump" },
	{ 0xabeedcfb, "__napi_schedule" },
	{ 0x6e6668b9, "alloc_netdev_mqs" },
	{ 0x1995b23e, "napi_complete_done" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xa4cbac48, "eth_type_trans" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x7e3233d5, "ether_setup" },
	{ 0xd60e87c0, "kmem_cache_alloc_trace" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0x37a0cba, "kfree" },
	{ 0x69acdf38, "memcpy" },
	{ 0x6128b5fc, "__printk_ratelimit" },
	{ 0x237de14e, "unregister_netdev" },
	{ 0x51021590, "consume_skb" },
	{ 0xcaa64d91, "skb_put" },
	{ 0x96917914, "__skb_pad" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "953EA373F49A4DB141000AF");
