#include "ir.h"

#define OP_STACK_MAX 4 * 1024 * 1024
#define CALL_STACK_MAX 1100000

#define POP()    \
  (*(--top));  \
  TRACE("POP: "); \
  trace_wasm_value(*top); \
  TRACE("\n");

#define PUSH(value) \
  (*top++ = value); \
  TRACE("PUSH: "); \
  trace_wasm_value(value); \
  TRACE("\n");

#define PEEK()      (*(top-1))

#define POP_FRAME()         (--frame)
#define PUSH_FRAME(value)   (*(frame++) = value)

typedef struct {
  const byte* ret_addr;
  uint32_t block_depth;
  uint32_t fn_idx;
  wasm_value_t* locals;
  wasm_value_t* op_ptr;
} wasm_context_t;

typedef struct {
  wasm_module_t* module;
  
  byte* mem;
  byte* mem_end;

  wasm_func_decl_t** table;
  wasm_func_decl_t** table_end;

  wasm_value_t* op_stack_base;
  wasm_context_t* call_stack_base;

  wasm_value_t* globals;

  uint32_t main_idx;
  int has_start;
  uint32_t start_idx;
  
} wasm_instance_t;


int module_instantiate(wasm_instance_t *module_inst, wasm_module_t *module);

