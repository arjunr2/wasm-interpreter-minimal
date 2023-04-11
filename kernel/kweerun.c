#include <linux/kernel.h>
#include <linux/module.h>

#include "klib.h"
#include "test.h"


int init_module(void) {
	printk(KERN_INFO "Starting WASM runtime\n");
	print_lib();
	run_tests();
	return 0;
}

void cleanup_module(void) {
	printk(KERN_INFO "Cleanup WASM runtime\n");
}

MODULE_LICENSE("GPL");
