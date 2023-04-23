#define LINUX

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include "test.h"
#include "parse.h"
#include "sample_files.h"
#include "instantiate.h"

#define TEST_NAME add0_wasm
#define LEN(tname) LEN_(tname)
#define LEN_(tname) tname##_len

int __init startup_runtime(void) {
	printk(KERN_INFO "Starting WASM runtime\n");
	buffer_t buf = {
    TEST_NAME,
    TEST_NAME,
    TEST_NAME + LEN(TEST_NAME)
  };

	wasm_module_t module = {0};
	if (parse(&module, buf) < 0) {
		ERR("Error parsing module\n");
    return 1;
	}

  wasm_instance_t module_inst = {0};
  if (module_instantiate(&module_inst, &module) < 0) {
    ERR("Error instantiating module\n");
    return 1;
  }

	return 0;
}

void __exit exit_runtime(void) {
	printk(KERN_INFO "Exit WASM runtime\n\n");
}

module_init(startup_runtime);
module_exit(exit_runtime);

MODULE_LICENSE("GPL");
