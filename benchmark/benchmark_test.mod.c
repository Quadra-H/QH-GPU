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
	{ 0xa8c16cf3, "module_layout" },
	{ 0x8356a354, "qhgpu_call_sync" },
	{ 0xf432dd3d, "__init_waitqueue_head" },
	{ 0x47be1158, "qhgpu_vmalloc" },
	{ 0x82b1fee1, "qhgpu_vfree" },
	{ 0x32c86d07, "qhgpu_free_request" },
	{ 0x475871bb, "qhgpu_alloc_request" },
	{ 0x27e1a049, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=qhgpu";


MODULE_INFO(srcversion, "468B7D0E24B53844D1EF37F");