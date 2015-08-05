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

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=hal_lib,rtapi,hostmot2";

MODULE_ALIAS("pci:v000010B5d00009030sv000010B5sd00003131bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00009054sv000010B5sd00003312bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00009030sv000010B5sd00003132bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00009054sv000010B5sd00003314bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00009054sv000010B5sd00003313bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00009054sv000010B5sd00003315bc*sc*i*");
MODULE_ALIAS("pci:v00002718d00005125sv00002718sd00005125bc*sc*i*");
MODULE_ALIAS("pci:v00002718d00006125sv00002718sd00006125bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00009054sv000010B5sd00003133bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00009054sv000010B5sd00003311bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00009054sv000010B5sd00003472bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00009054sv000010B5sd00003473bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00009056sv000010B5sd00003427bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00009056sv000010B5sd00003428bc*sc*i*");
MODULE_ALIAS("pci:v000010B5d00009056sv000010B5sd00003429bc*sc*i*");

MODULE_INFO(srcversion, "369339716FD70180561A5DD");
