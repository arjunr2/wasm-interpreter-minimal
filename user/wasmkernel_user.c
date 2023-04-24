#define LINUX

#include "test.h"
#include "parse.h"
#include "sample_files.h"
#include "instantiate.h"
#include "run.h"

#define TEST_NAME i32_load8_u1_wasm
#define LEN(tname) LEN_(tname)
#define LEN_(tname) tname##_len

int main() {
	printf("Starting WASM runtime\n");
	buffer_t buf = {
    TEST_NAME,
    TEST_NAME,
    TEST_NAME + LEN(TEST_NAME)
  };

	wasm_module_t module = {0};
	if (parse(&module, buf) < 0) {
		ERR("Error parsing module\n");
    goto error_parse;
	}

  wasm_instance_t module_inst = {0};
  if (module_instantiate(&module_inst, &module) < 0) {
    ERR("Error instantiating module\n");
    goto error;
  }

  uint32_t num_args = 1;
  wasm_value_t args[2] = { { .tag = WASM_TYPE_I32, .val.i32 = 204} };
  wasm_value_t result = run_wasm(&module_inst, num_args, args);

  print_wasm_value("Return Value: ", result);

  module_deinstantiate(&module_inst);
  module_free(&module);
	return 0;

error:
  module_deinstantiate(&module_inst);
error_parse:
  module_free(&module);
  return 1;
}
