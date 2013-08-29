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
	{ 0xed7f1fa9, "qhgpu_call_sync" },
	{ 0x5a60f091, "qhgpu_mmap_addr_pass" },
	{ 0xcf56f60f, "qhgpu_alloc_request" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x4b06d2e7, "complete" },
	{ 0x91715312, "sprintf" },
	{ 0x27e1a049, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=qhgpu";


MODULE_INFO(srcversion, "075FC4389AE845D3EF3DA90");
