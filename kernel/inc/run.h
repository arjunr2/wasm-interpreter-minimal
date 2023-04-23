#pragma once

#include "ir.h"
#include "instantiate.h"

wasm_value_t run_wasm(wasm_instance_t *module_inst, uint32_t num_args, wasm_value_t* args);


/* Interpreter run defines */

/*** Runtime Trap ***/
#define TRAP()  \
  ERR("Runtime Trap!\n"); \
  return_result = wasm_i32_value(0xDEADBEEF); \
  return return_result;

/*** Stack manipulation ***/
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
/*** ***/


/*** Operation Macros ***/

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

/*** ***/


/*** Debugging macros ***/
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

/*** ***/


/*** JUMP TABLE ***/
#define TARGET_LABEL_DEF(label) [label] = &&TARGET_##label

#define CREATE_TARGET_JUMP_TABLE()  \
  static void *target_jump_table[] = {  \
  TARGET_LABEL_DEF(WASM_OP_UNREACHABLE), \
  TARGET_LABEL_DEF(WASM_OP_NOP),  \
  TARGET_LABEL_DEF(WASM_OP_BLOCK),  \
  TARGET_LABEL_DEF(WASM_OP_LOOP), \
  TARGET_LABEL_DEF(WASM_OP_IF), \
  TARGET_LABEL_DEF(WASM_OP_ELSE), \
  TARGET_LABEL_DEF(WASM_OP_END),  \
  TARGET_LABEL_DEF(WASM_OP_BR), \
  TARGET_LABEL_DEF(WASM_OP_BR_IF),  \
  TARGET_LABEL_DEF(WASM_OP_BR_TABLE), \
  TARGET_LABEL_DEF(WASM_OP_RETURN), \
  TARGET_LABEL_DEF(WASM_OP_CALL), \
  TARGET_LABEL_DEF(WASM_OP_CALL_INDIRECT),  \
  TARGET_LABEL_DEF(WASM_OP_DROP), \
  TARGET_LABEL_DEF(WASM_OP_SELECT), \
  TARGET_LABEL_DEF(WASM_OP_LOCAL_GET),  \
  TARGET_LABEL_DEF(WASM_OP_LOCAL_SET),  \
  TARGET_LABEL_DEF(WASM_OP_LOCAL_TEE),  \
  TARGET_LABEL_DEF(WASM_OP_GLOBAL_GET), \
  TARGET_LABEL_DEF(WASM_OP_GLOBAL_SET), \
  TARGET_LABEL_DEF(WASM_OP_I64_LOAD), \
  TARGET_LABEL_DEF(WASM_OP_F32_LOAD), \
  TARGET_LABEL_DEF(WASM_OP_I32_LOAD), \
  TARGET_LABEL_DEF(WASM_OP_F64_LOAD), \
  TARGET_LABEL_DEF(WASM_OP_I32_LOAD8_S),  \
  TARGET_LABEL_DEF(WASM_OP_I32_LOAD8_U),  \
  TARGET_LABEL_DEF(WASM_OP_I32_LOAD16_S), \
  TARGET_LABEL_DEF(WASM_OP_I32_LOAD16_U), \
  TARGET_LABEL_DEF(WASM_OP_I64_LOAD8_S),  \
  TARGET_LABEL_DEF(WASM_OP_I64_LOAD8_U),  \
  TARGET_LABEL_DEF(WASM_OP_I64_LOAD16_S), \
  TARGET_LABEL_DEF(WASM_OP_I64_LOAD16_U), \
  TARGET_LABEL_DEF(WASM_OP_I64_LOAD32_S), \
  TARGET_LABEL_DEF(WASM_OP_I64_LOAD32_U), \
  TARGET_LABEL_DEF(WASM_OP_I32_STORE),  \
  TARGET_LABEL_DEF(WASM_OP_I64_STORE),  \
  TARGET_LABEL_DEF(WASM_OP_F32_STORE),  \
  TARGET_LABEL_DEF(WASM_OP_F64_STORE),  \
  TARGET_LABEL_DEF(WASM_OP_I32_STORE8), \
  TARGET_LABEL_DEF(WASM_OP_I32_STORE16),  \
  TARGET_LABEL_DEF(WASM_OP_I64_STORE8), \
  TARGET_LABEL_DEF(WASM_OP_I64_STORE16),  \
  TARGET_LABEL_DEF(WASM_OP_I64_STORE32),  \
  TARGET_LABEL_DEF(WASM_OP_MEMORY_SIZE),  \
  TARGET_LABEL_DEF(WASM_OP_MEMORY_GROW),  \
  TARGET_LABEL_DEF(WASM_OP_I32_CONST),  \
  TARGET_LABEL_DEF(WASM_OP_I64_CONST),  \
  TARGET_LABEL_DEF(WASM_OP_F32_CONST),  \
  TARGET_LABEL_DEF(WASM_OP_F64_CONST),  \
  TARGET_LABEL_DEF(WASM_OP_I32_EQZ),  \
  TARGET_LABEL_DEF(WASM_OP_I32_EQ), \
  TARGET_LABEL_DEF(WASM_OP_I32_NE), \
  TARGET_LABEL_DEF(WASM_OP_I32_LT_S), \
  TARGET_LABEL_DEF(WASM_OP_I32_LT_U), \
  TARGET_LABEL_DEF(WASM_OP_I32_GT_S), \
  TARGET_LABEL_DEF(WASM_OP_I32_GT_U), \
  TARGET_LABEL_DEF(WASM_OP_I32_LE_S), \
  TARGET_LABEL_DEF(WASM_OP_I32_LE_U), \
  TARGET_LABEL_DEF(WASM_OP_I32_GE_S), \
  TARGET_LABEL_DEF(WASM_OP_I32_GE_U), \
  TARGET_LABEL_DEF(WASM_OP_I64_EQZ),  \
  TARGET_LABEL_DEF(WASM_OP_I64_EQ), \
  TARGET_LABEL_DEF(WASM_OP_I64_NE), \
  TARGET_LABEL_DEF(WASM_OP_I64_LT_S), \
  TARGET_LABEL_DEF(WASM_OP_I64_LT_U), \
  TARGET_LABEL_DEF(WASM_OP_I64_GT_S), \
  TARGET_LABEL_DEF(WASM_OP_I64_GT_U), \
  TARGET_LABEL_DEF(WASM_OP_I64_LE_S), \
  TARGET_LABEL_DEF(WASM_OP_I64_LE_U), \
  TARGET_LABEL_DEF(WASM_OP_I64_GE_S), \
  TARGET_LABEL_DEF(WASM_OP_I64_GE_U), \
  TARGET_LABEL_DEF(WASM_OP_F32_EQ), \
  TARGET_LABEL_DEF(WASM_OP_F32_NE), \
  TARGET_LABEL_DEF(WASM_OP_F32_LT), \
  TARGET_LABEL_DEF(WASM_OP_F32_GT), \
  TARGET_LABEL_DEF(WASM_OP_F32_LE), \
  TARGET_LABEL_DEF(WASM_OP_F32_GE), \
  TARGET_LABEL_DEF(WASM_OP_F64_EQ), \
  TARGET_LABEL_DEF(WASM_OP_F64_NE), \
  TARGET_LABEL_DEF(WASM_OP_F64_LT), \
  TARGET_LABEL_DEF(WASM_OP_F64_GT), \
  TARGET_LABEL_DEF(WASM_OP_F64_LE), \
  TARGET_LABEL_DEF(WASM_OP_F64_GE), \
  TARGET_LABEL_DEF(WASM_OP_I32_CLZ),  \
  TARGET_LABEL_DEF(WASM_OP_I32_CTZ),  \
  TARGET_LABEL_DEF(WASM_OP_I32_POPCNT), \
  TARGET_LABEL_DEF(WASM_OP_I32_ADD),  \
  TARGET_LABEL_DEF(WASM_OP_I32_SUB),  \
  TARGET_LABEL_DEF(WASM_OP_I32_MUL),  \
  TARGET_LABEL_DEF(WASM_OP_I32_DIV_S),  \
  TARGET_LABEL_DEF(WASM_OP_I32_DIV_U),  \
  TARGET_LABEL_DEF(WASM_OP_I32_REM_S),  \
  TARGET_LABEL_DEF(WASM_OP_I32_REM_U),  \
  TARGET_LABEL_DEF(WASM_OP_I32_AND),  \
  TARGET_LABEL_DEF(WASM_OP_I32_OR), \
  TARGET_LABEL_DEF(WASM_OP_I32_XOR),  \
  TARGET_LABEL_DEF(WASM_OP_I32_SHL),  \
  TARGET_LABEL_DEF(WASM_OP_I32_SHR_S),  \
  TARGET_LABEL_DEF(WASM_OP_I32_SHR_U),  \
  TARGET_LABEL_DEF(WASM_OP_I32_ROTL), \
  TARGET_LABEL_DEF(WASM_OP_I32_ROTR), \
  TARGET_LABEL_DEF(WASM_OP_I64_CLZ),  \
  TARGET_LABEL_DEF(WASM_OP_I64_CTZ),  \
  TARGET_LABEL_DEF(WASM_OP_I64_POPCNT), \
  TARGET_LABEL_DEF(WASM_OP_I64_ADD),  \
  TARGET_LABEL_DEF(WASM_OP_I64_SUB),  \
  TARGET_LABEL_DEF(WASM_OP_I64_MUL),  \
  TARGET_LABEL_DEF(WASM_OP_I64_DIV_S),  \
  TARGET_LABEL_DEF(WASM_OP_I64_DIV_U),  \
  TARGET_LABEL_DEF(WASM_OP_I64_REM_S),  \
  TARGET_LABEL_DEF(WASM_OP_I64_REM_U),  \
  TARGET_LABEL_DEF(WASM_OP_I64_AND),  \
  TARGET_LABEL_DEF(WASM_OP_I64_OR), \
  TARGET_LABEL_DEF(WASM_OP_I64_XOR),  \
  TARGET_LABEL_DEF(WASM_OP_I64_SHL),  \
  TARGET_LABEL_DEF(WASM_OP_I64_SHR_S),  \
  TARGET_LABEL_DEF(WASM_OP_I64_SHR_U),  \
  TARGET_LABEL_DEF(WASM_OP_I64_ROTL), \
  TARGET_LABEL_DEF(WASM_OP_I64_ROTR), \
  TARGET_LABEL_DEF(WASM_OP_F32_ABS),  \
  TARGET_LABEL_DEF(WASM_OP_F32_NEG),  \
  TARGET_LABEL_DEF(WASM_OP_F32_CEIL), \
  TARGET_LABEL_DEF(WASM_OP_F32_FLOOR),  \
  TARGET_LABEL_DEF(WASM_OP_F32_TRUNC),  \
  TARGET_LABEL_DEF(WASM_OP_F32_NEAREST),  \
  TARGET_LABEL_DEF(WASM_OP_F32_SQRT), \
  TARGET_LABEL_DEF(WASM_OP_F32_ADD),  \
  TARGET_LABEL_DEF(WASM_OP_F32_SUB),  \
  TARGET_LABEL_DEF(WASM_OP_F32_MUL),  \
  TARGET_LABEL_DEF(WASM_OP_F32_DIV),  \
  TARGET_LABEL_DEF(WASM_OP_F32_MIN),  \
  TARGET_LABEL_DEF(WASM_OP_F32_MAX),  \
  TARGET_LABEL_DEF(WASM_OP_F32_COPYSIGN), \
  TARGET_LABEL_DEF(WASM_OP_F64_ABS),  \
  TARGET_LABEL_DEF(WASM_OP_F64_NEG),  \
  TARGET_LABEL_DEF(WASM_OP_F64_CEIL), \
  TARGET_LABEL_DEF(WASM_OP_F64_FLOOR),  \
  TARGET_LABEL_DEF(WASM_OP_F64_TRUNC),  \
  TARGET_LABEL_DEF(WASM_OP_F64_NEAREST),  \
  TARGET_LABEL_DEF(WASM_OP_F64_SQRT), \
  TARGET_LABEL_DEF(WASM_OP_F64_ADD),  \
  TARGET_LABEL_DEF(WASM_OP_F64_SUB),  \
  TARGET_LABEL_DEF(WASM_OP_F64_MUL),  \
  TARGET_LABEL_DEF(WASM_OP_F64_DIV),  \
  TARGET_LABEL_DEF(WASM_OP_F64_MIN),  \
  TARGET_LABEL_DEF(WASM_OP_F64_MAX),  \
  TARGET_LABEL_DEF(WASM_OP_F64_COPYSIGN), \
  TARGET_LABEL_DEF(WASM_OP_I32_WRAP_I64), \
  TARGET_LABEL_DEF(WASM_OP_I32_TRUNC_F32_S),  \
  TARGET_LABEL_DEF(WASM_OP_I32_TRUNC_F32_U),  \
  TARGET_LABEL_DEF(WASM_OP_I32_TRUNC_F64_S),  \
  TARGET_LABEL_DEF(WASM_OP_I32_TRUNC_F64_U),  \
  TARGET_LABEL_DEF(WASM_OP_I64_EXTEND_I32_S), \
  TARGET_LABEL_DEF(WASM_OP_I64_EXTEND_I32_U), \
  TARGET_LABEL_DEF(WASM_OP_I64_TRUNC_F32_S),  \
  TARGET_LABEL_DEF(WASM_OP_I64_TRUNC_F32_U),  \
  TARGET_LABEL_DEF(WASM_OP_I64_TRUNC_F64_S),  \
  TARGET_LABEL_DEF(WASM_OP_I64_TRUNC_F64_U),  \
  TARGET_LABEL_DEF(WASM_OP_F32_CONVERT_I32_S),  \
  TARGET_LABEL_DEF(WASM_OP_F32_CONVERT_I32_U),  \
  TARGET_LABEL_DEF(WASM_OP_F32_CONVERT_I64_S),  \
  TARGET_LABEL_DEF(WASM_OP_F32_CONVERT_I64_U),  \
  TARGET_LABEL_DEF(WASM_OP_F32_DEMOTE_F64), \
  TARGET_LABEL_DEF(WASM_OP_F64_CONVERT_I32_S),  \
  TARGET_LABEL_DEF(WASM_OP_F64_CONVERT_I32_U),  \
  TARGET_LABEL_DEF(WASM_OP_F64_CONVERT_I64_S),  \
  TARGET_LABEL_DEF(WASM_OP_F64_CONVERT_I64_U),  \
  TARGET_LABEL_DEF(WASM_OP_F64_PROMOTE_F32),  \
  TARGET_LABEL_DEF(WASM_OP_I32_REINTERPRET_F32),  \
  TARGET_LABEL_DEF(WASM_OP_I64_REINTERPRET_F64),  \
  TARGET_LABEL_DEF(WASM_OP_F32_REINTERPRET_I32),  \
  TARGET_LABEL_DEF(WASM_OP_F64_REINTERPRET_I64),  \
  };

