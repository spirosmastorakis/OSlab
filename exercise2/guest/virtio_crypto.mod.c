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
	{ 0x568fba06, "module_layout" },
	{ 0x68f58ca6, "cdev_del" },
	{ 0x9c10fd4, "cdev_init" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x4d884691, "malloc_sizes" },
	{ 0x9f6b4c62, "down_interruptible" },
	{ 0x21380c66, "virtqueue_kick" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xe1e39241, "virtqueue_get_buf" },
	{ 0x67dd8dca, "nonseekable_open" },
	{ 0x6395be94, "__init_waitqueue_head" },
	{ 0x71de9b3f, "_copy_to_user" },
	{ 0x8f64aa4, "_raw_spin_unlock_irqrestore" },
	{ 0x9021c4eb, "current_task" },
	{ 0x27e1a049, "printk" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0xd641eabf, "cdev_add" },
	{ 0xa654f489, "virtqueue_add_buf_gfp" },
	{ 0x78764f4e, "pv_irq_ops" },
	{ 0xd64d5fbd, "unregister_virtio_driver" },
	{ 0x1000e51, "schedule" },
	{ 0x43261dca, "_raw_spin_lock_irq" },
	{ 0x7a172b67, "kmem_cache_alloc_trace" },
	{ 0xd52bf1ce, "_raw_spin_lock" },
	{ 0x9327f5ce, "_raw_spin_lock_irqsave" },
	{ 0xcf21d241, "__wake_up" },
	{ 0xb6244511, "sg_init_one" },
	{ 0x37a0cba, "kfree" },
	{ 0x5c8b5ce8, "prepare_to_wait" },
	{ 0x71e3cecb, "up" },
	{ 0xfa66f77c, "finish_wait" },
	{ 0x77e2f33, "_copy_from_user" },
	{ 0xcb9dcb2, "register_virtio_driver" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=virtio_ring,virtio";

MODULE_ALIAS("virtio:d0000000Dv*");
