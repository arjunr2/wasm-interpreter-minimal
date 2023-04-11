#include <linux/kernel.h>
#include <linux/module.h>

void print_lib(void) {
	printk(KERN_INFO "Calling lib\n");
}
