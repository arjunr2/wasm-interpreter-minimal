#pragma once

#include "ir.h"

#define RET_ERR -1
#define RET_SUCCESS 0

typedef struct {
  byte op;
  byte* start_addr;
  byte* end_addr;
} block_list_t;

int parse(wasm_module_t *module, buffer_t buf);

void module_free(wasm_module_t *mod);
