#include "ir.h"
#include "common.h"


const char* wasm_section_name(byte code) {
  switch (code) {
  case WASM_SECT_TYPE: return "type";
  case WASM_SECT_IMPORT: return "import";
  case WASM_SECT_FUNCTION: return "function";
  case WASM_SECT_TABLE: return "table";
  case WASM_SECT_MEMORY: return "memory";
  case WASM_SECT_GLOBAL: return "global";
  case WASM_SECT_EXPORT: return "export";
  case WASM_SECT_START: return "start";
  case WASM_SECT_ELEMENT: return "element";
  case WASM_SECT_CODE: return "code";
  case WASM_SECT_DATA: return "data";
  case WASM_SECT_DATACOUNT: return "datacount";
  case WASM_SECT_CUSTOM: return "custom";
  default:
    return "unknown";
  }
}

void print_wasm_value(const char* prefix, wasm_value_t val) {
  switch (val.tag) {
  case WASM_TYPE_I32:
    PRINT("%s%d\n", prefix, val.val.i32);
    break;
  case WASM_TYPE_F64:
    PRINT("%s%lf\n", prefix, val.val.f64);
    break;
  case WASM_TYPE_EXTERNREF:
    PRINT("%s%p\n", prefix, val.val.ref);
    break;
  default:
    ERR("Invalid type: %d\n", val.tag);
  }
}

void trace_wasm_value(const char* prefix, wasm_value_t val) {
  switch (val.tag) {
  case WASM_TYPE_I32:
    TRACE("%s%d\n", prefix, val.val.i32);
    break;
  case WASM_TYPE_F64:
    TRACE("%s%lf\n", prefix, val.val.f64);
    break;
  case WASM_TYPE_EXTERNREF:
    TRACE("%s%p\n", prefix, val.val.ref);
    break;
  default:
    ERR("Invalid type: %d\n", val.tag);
  }
}

wasm_value_t wasm_i32_value(int32_t val) {
  wasm_value_t r = {
    .tag = WASM_TYPE_I32,
    .val.i32 = val,
  };
  return r;
}

wasm_value_t wasm_f64_value(double val) {
  wasm_value_t r = {
    .tag = WASM_TYPE_F64,
    .val.f64 = val,
  };
  return r;
}

wasm_value_t wasm_ref_value(void* val) {
  wasm_value_t r = {
    .tag = WASM_TYPE_EXTERNREF,
    .val.ref = val,
  };
  return r;
}
