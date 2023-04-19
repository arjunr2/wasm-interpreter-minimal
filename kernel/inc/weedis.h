#pragma once 

#include "common.h"
#include "ir.h"

typedef struct {
  byte op;
  byte* start_addr;
  byte* end_addr;
} block_list_t;


void disassemble(wasm_module_t* module, const byte* start, const byte* end);

void decode_expr(buffer_t *buf, bool flush, uint32_t base_tab, bool replace_last);
