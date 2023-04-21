#define LINUX

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include "test.h"
#include "parse.h"
#include "sample_files.h"

#define TEST_NAME add0_wasm
#define LEN(tname) LEN_(tname)
#define LEN_(tname) tname##_len

int __init startup_runtime(void) {
	printk(KERN_INFO "Starting WASM runtime\n");
	const byte* start, end;
	wasm_module_t module = {0};
	buffer_t buf = {
    TEST_NAME,
    TEST_NAME,
    TEST_NAME + LEN(TEST_NAME)
  };

	int result = parse(&module, buf);
	if (result < 0) {
		ERR("Error parsing module | Return: %d\n", result);
	}

	return 0;
}

void __exit exit_runtime(void) {
	printk(KERN_INFO "Exit WASM runtime\n\n");
}

module_init(startup_runtime);
module_exit(exit_runtime);

MODULE_LICENSE("GPL");
