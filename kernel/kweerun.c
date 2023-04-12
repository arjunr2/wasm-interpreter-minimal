#define LINUX

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include "klib.h"
#include "test.h"


int __init startup_runtime(void) {
	printk(KERN_INFO "Starting WASM runtime\n");
	print_lib();
	run_tests();
	return 0;
}

void __exit exit_runtime(void) {
	printk(KERN_INFO "Exit WASM runtime\n\n");
}

module_init(startup_runtime);
module_exit(exit_runtime);

MODULE_LICENSE("GPL");
