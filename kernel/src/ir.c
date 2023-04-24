#include "ir.h"
#include "common.h"

wasm_value_t parse_wasm_value(char* str) {
  int len = strlen(str);
  if (len > 0) {
    if (str[len-1] == 'd' || str[len-1] == 'D') {
      // treat the input as a double
      char* end = NULL;
      //double result = strtod(str, &end);
			double result = 1.53;
      if (end == (str + len - 1)) return wasm_f64_value(result);
    } else {
      // treat the input as an integer
      long result;
			if (!kstrtol(str, 10, &result)) {
				//if (end == (str + len)) return wasm_i32_value(result);
				return wasm_i32_value(result);
			}
    }
  }
  wasm_value_t orig_string = {
    .tag = WASM_TYPE_EXTERNREF,
    .val.ref = str,
  };
  return orig_string;
}


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
      PRINT("%s%d", prefix, val.val.i32);
      break;
    case WASM_TYPE_I64:
      PRINT("%s%lld", prefix, val.val.i64);
      break;
    case WASM_TYPE_F64:
      PRINT("%s%lf", prefix, val.val.f64);
      break;
    case WASM_TYPE_EXTERNREF:
      PRINT("%s%p", prefix, val.val.ref);
      break;
    default:
      ERR("Invalid type: %d\n", val.tag);
  }
}

void trace_wasm_value(const char* prefix, wasm_value_t val) {
  switch (val.tag) {
    case WASM_TYPE_I32:
      TRACE("%s%d", prefix, val.val.i32);
      break;
    case WASM_TYPE_I64:
      TRACE("%s%lld", prefix, val.val.i64);
      break;
    case WASM_TYPE_F64:
      TRACE("%s%lf", prefix, val.val.f64);
      break;
    case WASM_TYPE_EXTERNREF:
      TRACE("%s%p", prefix, val.val.ref);
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

wasm_value_t wasm_i64_value(int64_t val) {
  wasm_value_t r = {
    .tag = WASM_TYPE_I64,
    .val.i64 = val,
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
