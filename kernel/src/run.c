//#include <math.h>
//#include <limits.h>

#include "common.h"
#include "ir.h"
#include "native.h"
#include "run.h"
#include "instantiate.h"
#include "wasmdefs.h"

/*** Control Flow ***/
#define VALIDATE_OPCODE() \
  opcode_entry_t *entry = &opcode_table[opcode]; \
  if (entry->invalid) { \
    ERR("Unimplemented opcode %d (%s)\n", opcode, entry->mnemonic); \
    TRAP(); \
  } 

#define FETCH_OPCODE()  \
  opcode = RD_BYTE(); \
  VALIDATE_OPCODE();  \
  inst_ct++;

#define TARGET_OP(opc) TARGET_##opc : 

#define TARGET_FETCH() \
  FETCH_OPCODE(); \
  TRACE("[%lu|%08lx|(%p ; %p)][%u]: %s\n", inst_ct, \
        *ip - 1 - buf->start, *ip, fn->code_end, \
        block_depth, opcode_names[opcode]); \
  goto *target_jump_table[opcode];

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

/*** ***/

/*** Import macros ***/
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

/*** ***/
  

/* Native Import function sequence */
wasm_value_t invoke_native_function(wasm_instance_t *inst, wasm_value_t* ptr, 
      wasm_import_decl_t *import, wasm_sig_decl_t *sig) {
  if (strcmp(import->mod_name, "weewasm")) {
    ERR("Invalid import module \'%s\' for %s\n", import->mod_name, import->member_name);
  }

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

  if (!is_void_fn && (result.tag == WASM_TYPE_EXTERNREF) && (result.val.ref == NULL)) {
    ERR("Could not find import function: \'%s\':\'%s\'\n", import->mod_name, import->member_name);
  }
  return result;
}


wasm_value_t run_wasm(wasm_instance_t *module_inst, uint32_t num_args, wasm_value_t* args) {
  
  wasm_instance_t *inst = module_inst;

  buffer_t basebuf = {0};
  buffer_t *buf = &basebuf;

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
  for (uint32_t i = 0; i < num_args; i++) { 
    PUSH(args[i]); 
  }

  // Init main frame and jump to code section
  block_depth = 0;
  locals = inst->op_stack_base;

  *ip = fn->code_start;
  {
    buffer_t newbuf = {
      .start = fn->code_start,
      .end = fn->code_end
    };
    buf = newbuf;
  };

  //  Function body locals
  PUSH_FUNCTION_LOCALS();

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
  {
    buffer_t newbuf = {
      .start = fn->code_start,
      .end = fn->code_end
    };
    buf = newbuf;
  };

  //  Function body locals
  PUSH_FUNCTION_LOCALS();

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
  return return_result;
}

