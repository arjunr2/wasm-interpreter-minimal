#pragma once

#include <stdint.h>
#include "weewasm.h"

#define TRAP()  \
  printf("!trap\n");\
  exit(1);

#define OP_STACK_MAX 4 * 1024 * 1024
#define CALL_STACK_MAX 1100000

#define STACK_INIT(dst, type, sz) \
  dst = ( type * ) malloc(sz * sizeof( type ));  \

#define STACK_FREE(dst) \
  free(dst);

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


typedef uint8_t byte;

typedef enum {
  I32 = WASM_TYPE_I32, 
  F64 = WASM_TYPE_F64, 
  EXTERNREF = WASM_TYPE_EXTERNREF,
  FUNCREF = WASM_TYPE_FUNCREF
} wasm_type_t;

typedef enum {
  FUNC = WASM_IE_DESC_FUNC, 
  GLOBAL = WASM_IE_DESC_GLOBAL, 
  TABLE = WASM_IE_DESC_TABLE, 
  MEMORY = WASM_IE_DESC_MEM
} wasm_kind_t;

typedef struct {
  wasm_type_t tag;
  union {
    uint32_t i32;
    double f64;
    void* ref;
  } val;
} wasm_value_t;

wasm_value_t parse_wasm_value(char* string);
void print_wasm_value(wasm_value_t val);
void trace_wasm_value(wasm_value_t val);

wasm_value_t wasm_i32_value(int32_t val);
wasm_value_t wasm_f64_value(double val);
wasm_value_t wasm_ref_value(void* val);

typedef struct {
  uint32_t initial;
  uint32_t max;
  unsigned has_max : 1;
} wasm_limits_t;

typedef struct {
  uint32_t mod_name_length;
  const char* mod_name;
  uint32_t member_name_length;
  const char* member_name;
  wasm_kind_t kind;
  uint32_t index;
} wasm_import_decl_t;


typedef struct {
  uint32_t count;
  wasm_type_t type;
} wasm_local_decl_t;


typedef struct {
  uint32_t num_params;
  wasm_type_t* params;
  uint32_t num_results;
  wasm_type_t* results;
} wasm_sig_decl_t;

typedef struct {
  uint32_t sig_index;
  wasm_sig_decl_t* sig;
  uint32_t num_local_vec;
  uint32_t num_locals;
  wasm_local_decl_t *local_decl;
  const byte* code_start;
  const byte* code_end;
} wasm_func_decl_t;

typedef struct {
  wasm_limits_t limits;
} wasm_table_decl_t;

typedef struct {
  wasm_type_t type;
  unsigned mutable : 1;
  const byte* init_expr_start;
  const byte* init_expr_end;
} wasm_global_decl_t;

typedef struct {
  uint32_t mem_offset;
  const byte* bytes_start;
  const byte* bytes_end;
} wasm_data_decl_t;

typedef struct {
  uint32_t table_offset;
  uint32_t length;
  uint32_t* func_indexes;
} wasm_elems_decl_t;

typedef struct {
  uint32_t length;
  char *name;
  wasm_kind_t kind;
  uint32_t index;
} wasm_export_decl_t;



typedef struct {
  uint32_t num_mems;
  wasm_limits_t mem_limits;
  
  uint32_t num_tables;
  wasm_table_decl_t* table;
  
  uint32_t num_sigs;
  wasm_sig_decl_t* sigs;

  uint32_t num_imports;
  wasm_import_decl_t* imports;

  uint32_t num_funcs;
  wasm_func_decl_t* funcs;

  uint32_t num_exports;
  wasm_export_decl_t* exports;

  uint32_t num_globals;
  wasm_global_decl_t* globals;
  
  uint32_t num_data;
  wasm_data_decl_t* data;

  uint32_t num_elems;
  wasm_elems_decl_t* elems;

  int has_start;
  uint32_t start_idx;
} wasm_module_t;


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
