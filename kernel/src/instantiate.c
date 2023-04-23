#include "instantiate.h"
#include "common.h"

#define RET_ERR -1

static int retval = 0;

void memory_instantiate(wasm_instance_t *module_inst, wasm_module_t *module) {
  uint32_t num_pages = module->mem_limits.initial ? module->mem_limits.initial : 1;

  uint32_t init_mem_size = PAGE_SIZE * num_pages;

  MALLOC(memory, byte, init_mem_size);

  module_inst->mem = memory;
  module_inst->mem_end = memory + init_mem_size;
}

void data_instantiate(wasm_instance_t *module_inst, wasm_module_t *module) {
  for (uint32_t i = 0; i < module->num_datas; i++) {
    wasm_data_decl_t *data = &module->data[i];
    uint32_t offset = data->mem_offset;
    // Copy data into memory
    memcpy(module_inst->mem + offset, data->bytes_start, 
            data->bytes_end - data->bytes_start);
  }
}

//double read_double(buffer_t *buf) {
//  uint64_t u64 = read_u64(buf);
//  double dval;
//  memcpy(&dval, &u64, sizeof(dval));
//  return dval;
//}

wasm_value_t decode_const(buffer_t *buf) {
  byte opcode = RD_BYTE();
  wasm_value_t res = wasm_i32_value(0xDEADBEEF);
  switch (opcode) {
    case WASM_OP_I32_CONST: ;
      int32_t val = read_i32leb(buf);
      res = wasm_i32_value(val);
      break;

    default:
      ERR("Global expr must be a i32 constant\n");
      retval = RET_ERR;
  }
  return res;
}


void globals_instantiate(wasm_instance_t *module_inst, wasm_module_t *module) {
  MALLOC(globals, wasm_value_t, module->num_globals);
  
  for (uint32_t i = 0; i < module->num_globals; i++) {
    wasm_global_decl_t *global = &module->globals[i];
    buffer_t buf = {global->init_expr_start, 
                    global->init_expr_start, 
                    global->init_expr_end };
    globals[i] = decode_const(&buf);
  }

  module_inst->globals = globals;
}


void startup_instantiate(wasm_instance_t *module_inst, wasm_module_t *module) {
  // Check for exported main
  bool found_main = false;
  for (uint32_t i = 0; i < module->num_exports; i++) {
    wasm_export_decl_t *export = &module->exports[i];
    if ((strcmp(export->name, "main") == 0) &&
          export->kind == KIND_FUNC) {
      // Set main fn
      module_inst->main_idx = module->exports[i].index;
      found_main = true;
    }
  }

  if (!found_main) {
    ERR("Cannot find exported main!\n");
    retval = RET_ERR;
    return;
  }

  module_inst->has_start = module->has_start;
  module_inst->start_idx = module->start_idx;

  MALLOC(op_stack, wasm_value_t, OP_STACK_MAX);
  MALLOC(call_stack, wasm_context_t, CALL_STACK_MAX);
  module_inst->op_stack_base = op_stack;
  module_inst->call_stack_base = call_stack;
}

void table_instantiate(wasm_instance_t *module_inst, wasm_module_t *module) {
  if (module->num_tables) {
    uint32_t table_size = module->table->limits.initial;
    MALLOC(table, wasm_func_decl_t*, table_size);
    memset(table, 0, table_size * sizeof(wasm_func_decl_t*));
    for (uint32_t i = 0; i < module->num_elems; i++) {
      wasm_elems_decl_t *elem = &module->elems[i];
      for (uint32_t j = 0; j < elem->length; j++) {
        uint32_t fn_idx = elem->func_indexes[j];
        table[elem->table_offset + j] = &module->funcs[fn_idx];
      }
    }
    module_inst->table = table;
    module_inst->table_end = table + table_size;
  }
}

void imports_instantiate(wasm_instance_t *module_inst) {
  return;
}

// Instantiate a wasm module, including dynamic components
int module_instantiate(wasm_instance_t *module_inst, wasm_module_t *module) {
  retval = 0;

  module_inst->module = module;

  memory_instantiate(module_inst, module);
  data_instantiate(module_inst, module);
  globals_instantiate(module_inst, module);
  table_instantiate(module_inst, module);
  imports_instantiate(module_inst);

  startup_instantiate(module_inst, module);

  return retval;
}


void module_deinstantiate(wasm_instance_t *inst) {
  wasm_module_t *mod = inst->module;
  FREE(inst->mem, 1);
  FREE(inst->globals, mod->num_globals);
  FREE(inst->table, mod->num_tables);

  FREE(inst->op_stack_base, OP_STACK_MAX);
  FREE(inst->call_stack_base, CALL_STACK_MAX);
}

