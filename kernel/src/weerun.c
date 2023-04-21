#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <limits.h>

#include "common.h"
#include "weedis.h"
#include "test.h"
#include "ir.h"
#include "native.h"


/* Debugging macros */
#define TRACE_LOCAL_LIST() \
  TRACE("Num params: %u\n", fn->sig->num_params); \
  TRACE("-- Locals --\n");  \
  it = locals;  \
  while (it != op_ptr) {  \
    TRACE("%lu: %2x\n", op_ptr - it, it->tag); \
    it++; \
  }

#define STACK_TRACE() \
  wasm_value_t *itr = top - 1;  \
  TRACE("-- Op stack --\n");  \
  while (itr != inst->op_stack_base - 1) { \
    trace_wasm_value(*itr); \
    TRACE("\n");  \
    itr--;  \
  }

#define PUSH_FUNCTION_LOCALS() \
  for (uint32_t i = 0; i < fn->num_local_vec; i++) {  \
    for (uint32_t j = 0; j < fn->local_decl[i].count; j++) {  \
      wasm_value_t local = { .tag = fn->local_decl[i].type, .val = 0 }; \
      PUSH(local);  \
    } \
  } \
  op_ptr = top;

/*   */
  
/* Operation Macros */

#define BINARY_OP_I32(op, sgn)  \
  v1 = POP();  \
  v2 = POP();  \
  sgn##int32_t res = ((sgn##int32_t)v2.val.i32) op ((sgn##int32_t)v1.val.i32);  \
  PUSH(wasm_i32_value(res));

#define DIV_OP_I32() \
  v1 = POP();  \
  v2 = POP();  \
  if (v1.val.i32 == 0)  { TRAP(); } \
  if ((v2.val.i32 == INT_MIN) && (v1.val.i32 == -1)) { TRAP(); }  \
  int32_t res = ((int32_t)v2.val.i32) / ((int32_t)v1.val.i32);  \
  PUSH(wasm_i32_value(res));

#define DIV_OP_U32() \
  v1 = POP();  \
  v2 = POP();  \
  if (v1.val.i32 == 0)  { TRAP(); } \
  uint32_t res = ((uint32_t)v2.val.i32) / ((uint32_t)v1.val.i32);  \
  PUSH(wasm_i32_value(res));

#define REM_OP_I32() \
  v1 = POP();  \
  v2 = POP();  \
  if (v1.val.i32 == 0)  { TRAP(); } \
  if ((v2.val.i32 == INT_MIN) && (v1.val.i32 == -1)) { \
    PUSH(wasm_i32_value(0));  \
  } else {  \
    int32_t res = ((int32_t)v2.val.i32) % ((int32_t)v1.val.i32);  \
    PUSH(wasm_i32_value(res));  \
  }

#define REM_OP_U32() \
  v1 = POP();  \
  v2 = POP();  \
  if (v1.val.i32 == 0)  { TRAP(); } \
  uint32_t res = ((uint32_t)v2.val.i32) % ((uint32_t)v1.val.i32);  \
  PUSH(wasm_i32_value(res));


#define BINARY_OP_F64(op, ty)  \
  v1 = POP();  \
  v2 = POP();  \
  PUSH(wasm_##ty##_value(v2.val.f64 op v1.val.f64));

#define ROTATE_OP(op, rev_op) \
  v1 = POP(); \
  v2 = POP(); \
  uint32_t amt = v1.val.i32;  \
  uint32_t val = v2.val.i32;  \
  uint32_t res = (val op amt) | (val rev_op (32 - amt));  \
  PUSH(wasm_i32_value(res));


#define uEXT_I32(val, sz)  \
  val &= ((1 << sz)-1);

#define EXT_I32(val, sz)  \
  uint32_t sign_bit = (val >> (sz-1)) & 1; \
  if (sign_bit) {  \
    val |= ((1 << (32 - sz)) - 1) << sz;\
  } else { \
    uEXT_I32(val, sz);  \
  } \
  

#define GET_ADDR(bytes)  \
  uint32_t align = read_u32leb(buf);  \
  uint32_t off = read_u32leb(buf);  \
  v1 = POP(); \
  uint32_t addr = v1.val.i32; \
  uint32_t maddr = addr + off; \
  if (((inst->mem + addr) + off + bytes) > inst->mem_end) { \
    TRAP(); \
  } \
  

#define LOAD_I32_OP(sz, sgn) \
  GET_ADDR(sz/8); \
  sgn##int##sz##_t res = *((sgn##int##sz##_t *)(inst->mem + maddr)); \
  /*sgn##EXT_I32(res, sz);*/  \
  TRACE("Load: %08x\n", res);  \
  PUSH(wasm_i32_value(res));


#define STORE_I32_OP(sz) \
  v2 = POP(); \
  GET_ADDR(sz/8); \
  uint##sz##_t sval = v2.val.i32 & ((1 << sz) - 1); \
  *(uint##sz##_t *)(inst->mem + maddr) = sval; \


#define LOAD_F64_OP() \
  GET_ADDR(8); \
  double res = *((double *)(inst->mem + maddr)); \
  wasm_value_t vp = wasm_f64_value(res); \
  TRACE("Load: "); trace_wasm_value(vp); TRACE("\n");  \
  PUSH(vp);


#define STORE_F64_OP() \
  v2 = POP(); \
  GET_ADDR(8); \
  double sval = v2.val.f64; \
  *(double *)(inst->mem + maddr) = sval;


#define TRUNC_U() \
  v1 = POP(); \
  double tres = trunc(v1.val.f64);  \
  uint64_t res = (uint64_t) tres; \
  if (res >> 32) { TRAP(); } \
  PUSH(wasm_i32_value(res)); 

#define TRUNC_S() \
  v1 = POP(); \
  double tres = trunc(v1.val.f64);  \
  int64_t res = (int64_t) tres; \
  if ((res > INT_MAX) || (res < INT_MIN)) { TRAP(); } \
  PUSH(wasm_i32_value(res)); 
  
#define CONVERT(sgn)  \
  v1 = POP(); \
  double res = (double)((sgn##int32_t) v1.val.i32);  \
  PUSH(wasm_f64_value(res)); 
  


#define FETCH_OPCODE()  opcode = read_u8(buf); inst_ct++;

#define TARGET_OP(opc) TARGET_##opc : 

#define TARGET_FETCH() \
  FETCH_OPCODE(); \
  TRACE("[%lu|%08lx|(%p ; %p)][%u]: %s\n", inst_ct, \
        *ip - 1 - buf->start, *ip, fn->code_end, \
        block_depth, opcode_names[opcode]); \
  goto *target_jump_table[opcode];


/* Import macros */
#define EXPAND_ARGS3(a, b, l)   EXPAND_ARGS2(a, b), ptr[2].val.l
#define EXPAND_ARGS2(a, l)      EXPAND_ARGS1(a), ptr[1].val.l
#define EXPAND_ARGS1(l)         ptr[0].val.l 
#define EXPAND_ARGS0()    

#define EXPAND2_ARGS3()       ptr[0], ptr[1], ptr[2]
#define EXPAND2_ARGS2()       ptr[0], ptr[1]
#define EXPAND2_ARGS1()       ptr[0] 
#define EXPAND2_ARGS0()    

#define IMPORT_CHECK_CALL(name, rty, fn, num, args...) \
  if (!strcmp(import->member_name, name)) { \
    result = wasm_##rty##_value( fn(EXPAND_ARGS##num(args)) ); \
  }
  
#define IMPORT_CHECK_CALL2(name, fn, numparams)  \
  if (!strcmp(import->member_name, name)) { \
    result = fn(EXPAND2_ARGS##numparams()); \
  } 
  
#define IMPORT_CHECK_CALL2_VOID(name, fn, numparams)  \
  if (!strcmp(import->member_name, name)) { \
    fn(EXPAND2_ARGS##numparams()); \
    is_void_fn = true;  \
  } 

#define CALL_ROUTINE()  \
    /* If it is an import, call function and push result on stack */  \
    if (next_fn_idx < inst->module->num_imports) {  \
      wasm_import_decl_t* import = &inst->module->imports[next_fn_idx]; \
      wasm_sig_decl_t* sig = call_fn->sig;  \
      top -= sig->num_params; \
      wasm_value_t result = invoke_native_function(inst, top, import, sig); \
      if (sig->num_results != 0) {  PUSH(result); } \
      TARGET_FETCH(); \
    } \
    /* Save current function context */ \
    wasm_context_t ctx = {  \
      .ret_addr = *ip,  \
      .block_depth = block_depth, \
      .fn_idx = fn_idx, \
      .locals = locals, \
      .op_ptr = op_ptr  \
    };  \
    PUSH_FRAME(ctx);  \
                      \
    /* Init new frame and jump to code section */ \
    fn_idx = next_fn_idx; \
    fn = call_fn; \
    block_depth = 0;  \
    locals = top - fn->sig->num_params; \
    *ip = fn->code_start; \
                      \
    /* Function body locals */  \
    PUSH_FUNCTION_LOCALS(); \
                      \
    /* Trace debugging */ \
    //TRACE_LOCAL_LIST(); \
  

// Disassembles and runs a wasm module.
wasm_value_t run_wasm(const byte* start, const byte* end, uint32_t num_args, wasm_value_t* args);


// A pair of a buffer and a module into which to parse the bytes.
typedef struct {
  buffer_t* buf;
  wasm_module_t* module;
} mparse_t;

//=== SOLUTION CODE -- YOUR SOLUTION HERE ==================================
int parse(wasm_module_t* module, buffer_t* buf);


void memory_instantiate(wasm_instance_t *module_inst, wasm_module_t *module) {
  uint32_t num_pages = module->mem_limits.initial ? module->mem_limits.initial : 1;

  uint32_t init_mem_size = WASM_PAGE_SIZE * num_pages;

  MALLOC(memory, byte, init_mem_size);

  module_inst->mem = memory;
  module_inst->mem_end = memory + init_mem_size;
}

void data_instantiate(wasm_instance_t *module_inst, wasm_module_t *module) {
  for (uint32_t i = 0; i < module->num_data; i++) {
    wasm_data_decl_t *data = &module->data[i];
    uint32_t offset = data->mem_offset;
    // Copy data into memory
    memcpy(module_inst->mem + offset, data->bytes_start, 
            data->bytes_end - data->bytes_start);
  }
}

double read_double(buffer_t *buf) {
  uint64_t u64 = read_u64(buf);
  double dval;
  memcpy(&dval, &u64, sizeof(dval));
  return dval;
}

wasm_value_t decode_const(buffer_t *buf) {
  byte opcode = read_u8(buf);
  wasm_value_t res = wasm_i32_value(0xDEADBEEF);
  switch (opcode) {
    case WASM_OP_I32_CONST: ;
      int32_t val = read_i32leb(buf);
      res = wasm_i32_value(val);
      break;

    case WASM_OP_F64_CONST: ;
      double v = read_double(buf);
      res = wasm_f64_value(v);
      break;

    default:
      ERR("Global expr must be a constant\n");
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
    TRACE("Global %d: ", i);
    trace_wasm_value(globals[i]);
    TRACE("\n");
  }

  module_inst->globals = globals;
}


void startup_instantiate(wasm_instance_t *module_inst, wasm_module_t *module) {
  // Check for exported main
  bool found_main = false;
  for (uint32_t i = 0; i < module->num_exports; i++) {
    wasm_export_decl_t *export = &module->exports[i];
    if ((strcmp(export->name, "main") == 0) &&
          export->kind == FUNC) {
      // Set main fn
      module_inst->main_idx = module->exports[i].index;
      found_main = true;
    }
  }

  if (!found_main) {
    ERR("Cannot find exported main!\n");
    exit(1);
  }

  module_inst->has_start = module->has_start;
  module_inst->start_idx = module->start_idx;

  STACK_INIT(module_inst->op_stack_base, wasm_value_t, OP_STACK_MAX);
  STACK_INIT(module_inst->call_stack_base, wasm_context_t, CALL_STACK_MAX);
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
wasm_instance_t module_instantiate(wasm_module_t *module) {
  wasm_instance_t module_inst;

  module_inst.module = module;

  memory_instantiate(&module_inst, module);
  data_instantiate(&module_inst, module);
  globals_instantiate(&module_inst, module);
  table_instantiate(&module_inst, module);
  imports_instantiate(&module_inst);

  startup_instantiate(&module_inst, module);

  return module_inst;
}


void module_deinstantiate(wasm_instance_t *inst) {
  wasm_module_t *mod = inst->module;
  FREE(inst->mem, 1);
  FREE(inst->globals, mod->num_globals);
  FREE(inst->table, mod->num_tables);

  STACK_FREE(inst->op_stack_base);
  STACK_FREE(inst->call_stack_base);
}

void module_free(wasm_module_t* mod) {

  FREE(mod->table, mod->num_tables);

  for (uint32_t i = 0; i < mod->num_sigs; i++) {
    FREE(mod->sigs[i].params, mod->sigs[i].num_params);
    FREE(mod->sigs[i].results, mod->sigs[i].num_results);
  }
  FREE(mod->sigs, mod->num_sigs);

  for (uint32_t i = 0; i < mod->num_imports; i++) {
    FREE(mod->imports[i].mod_name, mod->imports[i].mod_name_length);
    FREE(mod->imports[i].member_name, mod->imports[i].member_name_length);
  }
  FREE(mod->imports, mod->num_imports);

  for (uint32_t i = 0; i < mod->num_funcs; i++) {
    uint32_t idx = i + mod->num_imports;
    FREE(mod->funcs[idx].local_decl, mod->funcs[idx].num_local_vec);
  }
  FREE(mod->funcs, mod->num_funcs);

  for (uint32_t i = 0; i < mod->num_exports; i++) {
    FREE(mod->exports[i].name, mod->exports[i].length);
  }
  FREE(mod->exports, mod->num_exports);

  FREE(mod->globals, mod->num_globals);

  FREE(mod->data, mod->num_data);

  for (uint32_t i = 0; i < mod->num_elems; i++) {
    FREE(mod->elems[i].func_indexes, mod->elems[i].length);
  }
  FREE(mod->elems, mod->num_elems);

}


/* Native Import function sequence */
wasm_value_t invoke_native_function(wasm_instance_t *inst, wasm_value_t* ptr, 
      wasm_import_decl_t *import, wasm_sig_decl_t *sig) {
  if (strcmp(import->mod_name, "weewasm")) {
    ERR("Invalid import module \'%s\' for %s\n", import->mod_name, import->member_name);
  }
  uint32_t num_params = sig->num_params;
  uint32_t num_results = sig->num_results;

  wasm_value_t result = wasm_ref_value(NULL);
  bool is_void_fn = false;

  IMPORT_CHECK_CALL2("obj.new",      native_obj_new,        0);
  IMPORT_CHECK_CALL2("obj.box_i32",  native_obj_box_i32,    1);
  IMPORT_CHECK_CALL2("obj.box_f64",  native_obj_box_f64,    1);

  IMPORT_CHECK_CALL2("obj.get",      native_obj_get,        2);
  IMPORT_CHECK_CALL2_VOID("obj.set",      native_obj_set,        3);

  IMPORT_CHECK_CALL2("i32.unbox",    native_i32_unbox,      1);
  IMPORT_CHECK_CALL2("f64.unbox",    native_f64_unbox,      1);
  
  IMPORT_CHECK_CALL2("obj.eq",       native_obj_eq,         2);

  if (!is_void_fn && (result.tag == EXTERNREF) && (result.val.ref == NULL)) {
    ERR("Could not find import function: \'%s\':\'%s\'\n", import->mod_name, import->member_name);
  }
  return result;
}


wasm_value_t run_wasm(const byte* start, const byte* end, uint32_t num_args, wasm_value_t* args) {
  buffer_t onstack_buf = {
    start,
    start,
    end
  };
  wasm_module_t onstack_module;
  memset(&onstack_module, 0, sizeof(onstack_module));
  int result = parse(&onstack_module, &onstack_buf);
  if (result < 0) return wasm_i32_value(-1);

  wasm_instance_t module_inst = module_instantiate(&onstack_module);
  wasm_instance_t *inst = &module_inst;


  buffer_t *buf = &onstack_buf;
  buf->ptr = start;

  /* Construct jump table*/
  CREATE_TARGET_JUMP_TABLE();

  // Setup global dynamic state
  uint32_t next_fn_idx;
  uint64_t inst_ct = 0;
  byte opcode;
  wasm_value_t return_result  = wasm_i32_value(0xDEADBEEF);
  wasm_value_t *top           = inst->op_stack_base;
  wasm_context_t *frame       = inst->call_stack_base;

  // Setup current frame state
  const byte **ip         = &buf->ptr;
  uint32_t fn_idx;
  wasm_func_decl_t *fn;
  uint32_t block_depth;
  wasm_value_t *locals;
  wasm_value_t *op_ptr;

  // Temp vars
  wasm_value_t v1, v2, v3, v4;
  wasm_value_t *it;
  
  // Jump to start/main
  if (inst->has_start) { goto start_init; };

main_init:
  top = inst->op_stack_base;
  frame = inst->call_stack_base;

  // Check for correct arguments
  fn_idx = inst->main_idx;
  fn = &inst->module->funcs[fn_idx];
  if (fn->sig->num_params != num_args) {
    ERR("Invalid number of arguments to main\n");
    exit(1);
  }
  for (uint32_t i = 0; i < fn->sig->num_params; i++) {
    if (fn->sig->params[i] != args[i].tag) {
      ERR("Invalid argument types for main\n");
      exit(1);
    }
  }

  // Main argument locals
  for (uint32_t i = 0; i < num_args; i++) { PUSH(args[i]); }

  // Init main frame and jump to code section
  block_depth = 0;
  locals = inst->op_stack_base;
  *ip = fn->code_start;

  //  Function body locals
  PUSH_FUNCTION_LOCALS();
  //TRACE_LOCAL_LIST();

  // Fetch first opcode
  TARGET_FETCH();


start_init:
  top = inst->op_stack_base;
  frame = inst->call_stack_base;

  // Check for correct arguments
  fn_idx = inst->start_idx;
  fn = &inst->module->funcs[fn_idx];

  // Start argument locals
  for (uint32_t i = 0; i < num_args; i++) { PUSH(args[i]); }

  // Init main frame and jump to code section
  block_depth = 0;
  locals = inst->op_stack_base;
  *ip = fn->code_start;

  //  Function body locals
  PUSH_FUNCTION_LOCALS();
  //TRACE_LOCAL_LIST();

  // Fetch first opcode
  TARGET_FETCH();




  TARGET_OP(WASM_OP_UNREACHABLE) { 
    ERR("Unreachable target at addr %08lx!\n", (*ip - 1 - buf->start));
    TRAP();
    TARGET_FETCH();
  }

  TARGET_OP(WASM_OP_NOP) { 
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_BLOCK) { 
    uint32_t blocktype = read_u32leb(buf);
    block_depth++;
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_LOOP) { 
    uint32_t blocktype = read_u32leb(buf);
    block_depth++;
    TARGET_FETCH();
  }

  // Not used
  TARGET_OP(WASM_OP_IF) { 
    TARGET_FETCH();
  }

  // Not used
  TARGET_OP(WASM_OP_ELSE) { 
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_END) { 
    block_depth--;
    if (*ip == fn->code_end)
      goto TARGET_WASM_OP_RETURN;
    
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_BR) { 
    int32_t imm = read_u32(buf);
    *ip += imm;
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_BR_IF) {
    int32_t imm = read_u32(buf);
    v1 = POP();
    if (v1.val.i32) { *ip += imm; }
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_BR_TABLE) { 
    v1 = POP();
    uint32_t num_elems = read_u32leb(buf);
    uint32_t idx = v1.val.i32;
    uint32_t idx_ch = (idx < num_elems) ? idx : num_elems;
    buf->ptr += (4 * idx_ch);
    int32_t offset = (int32_t)(read_u32(buf));
    *ip += offset;
    TRACE("OFFSET: %d\n", offset);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_RETURN) { 
    /* Reset stack and push result from fn */
    if (fn->sig->num_results == 1) {
      return_result = POP();
      top = locals;
      PUSH(return_result);
    } else {
      top = locals;
    }

    /* Check if exits main */
    if (frame == inst->call_stack_base) {
      goto exit_interp_loop;
    }

    POP_FRAME();
    // Return to original address
    *ip = frame->ret_addr;
    block_depth = frame->block_depth;
    fn_idx = frame->fn_idx;
    fn = &inst->module->funcs[fn_idx];
    locals = frame->locals;
    op_ptr = frame->op_ptr;


    STACK_TRACE();
    // Jump to new function
    TARGET_FETCH(); 
  }


  TARGET_OP(WASM_OP_CALL) { 
    STACK_TRACE();
    next_fn_idx = read_u32leb(buf);

    wasm_func_decl_t *call_fn = &inst->module->funcs[next_fn_idx];
    
    CALL_ROUTINE();

    TARGET_FETCH(); 
  }


  TARGET_OP(WASM_OP_CALL_INDIRECT) { 
    uint32_t type_idx = read_u32leb(buf);
    uint32_t table_num = read_u32leb(buf);
    v1 = POP();
    uint32_t idx = v1.val.i32;

    if (inst->table + idx >= inst->table_end) { TRAP(); }

    wasm_func_decl_t *call_fn = inst->table[idx];
    next_fn_idx = call_fn - inst->module->funcs;
    // Dynamic type check
    if ((call_fn == NULL) || (call_fn->sig_index != type_idx)) { TRAP(); }

    CALL_ROUTINE();

    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_DROP) { 
    POP();
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_SELECT) { 
    v1 = POP();
    v2 = POP();
    v3 = POP();
    v4 = (v1.val.i32 ? v3 : v2);
    PUSH(v4);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_LOCAL_GET) { 
    uint32_t idx = read_u32leb(buf);
    PUSH(locals[idx]);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_LOCAL_SET) { 
    uint32_t idx = read_u32leb(buf);
    locals[idx] = POP();
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_LOCAL_TEE) { 
    uint32_t idx = read_u32leb(buf);
    locals[idx] = PEEK();
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_GLOBAL_GET) { 
    uint32_t idx = read_u32leb(buf);
    PUSH(inst->globals[idx]);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_GLOBAL_SET) { 
    uint32_t idx = read_u32leb(buf);
    inst->globals[idx] = POP();
    TARGET_FETCH(); 
  }

  // TODO
  TARGET_OP(WASM_OP_TABLE_GET) { 
    TARGET_FETCH(); 
  }

  // TODO
  TARGET_OP(WASM_OP_TABLE_SET) { 
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_LOAD) { 
    LOAD_I32_OP(32, );
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_LOAD) { 
    LOAD_F64_OP();
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_LOAD8_S) { 
    LOAD_I32_OP(8, );
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_LOAD8_U) { 
    LOAD_I32_OP(8, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_LOAD16_S) { 
    LOAD_I32_OP(16, );
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_LOAD16_U) { 
    LOAD_I32_OP(16, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_STORE) { 
    STORE_I32_OP(32);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_STORE) { 
    STORE_F64_OP();
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_STORE8) { 
    STORE_I32_OP(8);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_STORE16) { 
    STORE_I32_OP(16);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_CONST) { 
    int32_t v1 = read_i32leb(buf);
    PUSH(wasm_i32_value(v1));
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_CONST) { 
    double v1 = read_double(buf);
    PUSH(wasm_f64_value(v1));
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_EQZ) { 
    v1 = POP();
    PUSH(wasm_i32_value(v1.val.i32 == 0));
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_EQ) { 
    BINARY_OP_I32(==, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_NE) { 
    BINARY_OP_I32(!=, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_LT_S) { 
    BINARY_OP_I32(<, );
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_LT_U) { 
    BINARY_OP_I32(<, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_GT_S) { 
    BINARY_OP_I32(>, );
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_GT_U) { 
    BINARY_OP_I32(>, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_LE_S) { 
    BINARY_OP_I32(<=, );
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_LE_U) { 
    BINARY_OP_I32(<=, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_GE_S) { 
    BINARY_OP_I32(>=, );
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_GE_U) { 
    BINARY_OP_I32(>=, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_EQ) { 
    BINARY_OP_F64(==, i32);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_NE) { 
    BINARY_OP_F64(!=, i32);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_LT) { 
    BINARY_OP_F64(<, i32);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_GT) { 
    BINARY_OP_F64(>, i32);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_LE) { 
    BINARY_OP_F64(<=, i32);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_GE) { 
    BINARY_OP_F64(>=, i32);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_CLZ) {
    v1 = POP();
    uint32_t res = v1.val.i32 ? __builtin_clz(v1.val.i32) : 32;
    PUSH(wasm_i32_value(res));
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_CTZ) { 
    v1 = POP();
    uint32_t res = v1.val.i32 ? __builtin_ctz(v1.val.i32) : 32;
    PUSH(wasm_i32_value(res));
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_POPCNT) { 
    v1 = POP();
    uint32_t res = __builtin_popcount(v1.val.i32);
    PUSH(wasm_i32_value(res));
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_ADD) { 
    BINARY_OP_I32(+, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_SUB) { 
    BINARY_OP_I32(-, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_MUL) { 
    BINARY_OP_I32(*, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_DIV_S) { 
    DIV_OP_I32( );
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_DIV_U) { 
    DIV_OP_U32();
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_REM_S) { 
    REM_OP_I32();
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_REM_U) { 
    REM_OP_U32();
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_AND) { 
    BINARY_OP_I32(&, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_OR) { 
    BINARY_OP_I32(|, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_XOR) { 
    BINARY_OP_I32(^, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_SHL) { 
    BINARY_OP_I32(<<, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_SHR_S) { 
    BINARY_OP_I32(>>, );
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_SHR_U) { 
    BINARY_OP_I32(>>, u);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_ROTL) { 
    ROTATE_OP(<<, >>);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_ROTR) { 
    ROTATE_OP(>>, <<);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_ADD) { 
    BINARY_OP_F64(+, f64);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_SUB) { 
    BINARY_OP_F64(-, f64);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_MUL) { 
    BINARY_OP_F64(*, f64);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_DIV) { 
    BINARY_OP_F64(/, f64);
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_TRUNC_F64_S) { 
    TRUNC_S();
    TARGET_FETCH();
  }

  TARGET_OP(WASM_OP_I32_TRUNC_F64_U) { 
    TRUNC_U();
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_CONVERT_I32_S) { 
    CONVERT();
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_F64_CONVERT_I32_U) { 
    CONVERT(u);
    TARGET_FETCH(); 
  }

  // TODO
  TARGET_OP(WASM_OP_F64_CONVERT_I64_S) { 
    TARGET_FETCH(); 
  }

  // TODO
  TARGET_OP(WASM_OP_F64_CONVERT_I64_U) { 
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_EXTEND8_S) { 
    v1 = POP();
    uint32_t res = v1.val.i32;
    EXT_I32(res, 8);
    PUSH(wasm_i32_value(res));
    TARGET_FETCH(); 
  }

  TARGET_OP(WASM_OP_I32_EXTEND16_S) { 
    v1 = POP();
    uint32_t res = v1.val.i32;
    EXT_I32(res, 16);
    PUSH(wasm_i32_value(res));
    TARGET_FETCH(); 
  }

exit_interp_loop: ;
  if (inst->has_start && (fn_idx == inst->start_idx)) { 
    TRACE("Finished start method; jumping to main\n");
    goto main_init; 
  }
  module_deinstantiate(inst);
  module_free(&onstack_module);
  return return_result;
}


// The main disassembly routine.
int parse(wasm_module_t* module, buffer_t* buf) {
  disassemble(module, buf->start, buf->end);
  return 0;
}
