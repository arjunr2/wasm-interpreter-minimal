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
	{ 0xdd8f8694, "module_layout" },
	{ 0xbbea7e99, "nf_unregister_net_hook" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xa916b694, "strnlen" },
	{ 0x62a38e34, "nf_register_net_hook" },
	{ 0x6565e06e, "sock_release" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0x5bf5839c, "sock_create_kern" },
	{ 0x30cb0399, "init_net" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x8f2c9fb7, "kernel_sendmsg" },
	{ 0xc5850110, "printk" },
	{ 0x1b6314fd, "in_aton" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "2373CD5E97D9C63D367BD13");
