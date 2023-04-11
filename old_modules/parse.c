#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "common.h"
#include "vm.h"
#include "test.h"
#include "weewasm.h"
#include "illegal.h"
#include "ir.h"
#include "disass.h"

#define CHECK(x) do { if(!(x)) return -2; } while(0)

// Read a string and print its length and UTF-8 chars.
int read_and_copy_string(buffer_t* buf, const char** nameptr, const byte* sectend) {
  uint32_t namelen = read_u32leb(buf);
  DISASS("%u ", namelen);
  if (nameptr != NULL) *nameptr = strndup((const char*)buf->ptr, namelen);
  CHECK((sectend - buf->ptr) >= namelen);
  DISASS("\"%.*s\"", namelen, buf->ptr);
  buf->ptr += namelen;
  return 0;
}

wasm_type_t read_value_type(buffer_t* buf) {
  int32_t type = read_i32leb(buf);
  DISASS(" %s", type_name(type));
  switch (type) {
  case WASM_TYPE_I32: return I32;
  case WASM_TYPE_F64: return F64;
  case WASM_TYPE_EXTERNREF: return EXTERNREF;
  default:
    ERR("!illegal type (%s)", type_name(type));
    return EXTERNREF;
  }
}

wasm_value_t read_init_expr(buffer_t* buf) {
  wasm_value_t val = wasm_i32_value(0);
  byte b = read_u8(buf);
  switch (b) {
  case WASM_OP_I32_CONST: {
    val =  wasm_i32_value(read_i32leb(buf));
    break;
  }
  case WASM_OP_F64_CONST: {
    double* p = (double*)buf->ptr;
    buf->ptr += 8;
    val = wasm_f64_value(*p);
    break;
  }
  default: {
    ERR("!illegal init expr bytecode 0x%02X (%s)", b, bytecode_name(b));
    break;
  }
  }
  b = read_u8(buf);
  if (b != WASM_OP_END) ERR("!illegal init expr bytecode 0x%02X", b);
  return val;
}

// Read minimimum and optional maximum.
void read_limits(buffer_t* buf, wasm_limits_t* dest) {
  byte val = read_u8(buf);
  dest->initial = read_u32leb(buf);
  if ((val & 1) == 1) {
    dest->max = read_u32leb(buf);
    dest->has_max = 1;
    DISASS(" %u %u \t\t\t; initial and max\n", dest->initial, dest->max);
  } else {
    dest->max = 0xFFFFFFFFu;
    dest->has_max = 0;
    DISASS(" %u \t\t\t; initial\n", dest->initial);
  }
}

// Reads a section code, skipping custom sections.
int read_section_code(buffer_t* buf, wasm_module_t* module, uint32_t* sectlen) {
  while (buf->ptr < buf->end) {
    byte code = read_u8(buf);
    if (code != 0) {
      DISASS("%s ", section_name(code));
      // read a normal section
      *sectlen = read_u32leb(buf);
      CHECK((buf->end - buf->ptr) >= *sectlen);
      return code;
    }
    // print custom section
    DISASS("custom ");
    uint32_t sectlen = read_u32leb(buf);
    DISASS("%u ", sectlen);
    CHECK((buf->end - buf->ptr) >= sectlen);
    const byte* sectend = buf->ptr + sectlen;
    read_and_copy_string(buf, NULL, sectend);
    DISASS("\n");
    buf->ptr = sectend;
  }
  // reached the end of the file, or there was a nested error
  return -1;
}

void read_type_decl(buffer_t* buf, wasm_sig_decl_t* dest, const byte* sectend) {
  byte code = read_u8(buf);
  DISASS(" %s", type_decl_name(code));
  if (code != 0x60) ERR("!expected signature declaration (0x60), got 0x%02X", code);
  uint32_t pcount = read_u32leb(buf);
  DISASS(" %u", pcount);
  dest->num_params = pcount;
  dest->params = (wasm_type_t*)malloc(pcount * sizeof(wasm_type_t));
  for (uint32_t i = 0; i < pcount && (buf->ptr < buf->end); i++) {
    wasm_type_t t = read_value_type(buf);
    dest->params[i] = t;
  }
  uint32_t rcount = read_u32leb(buf);
  dest->num_results = rcount;
  dest->results = (wasm_type_t*)malloc(rcount * sizeof(wasm_type_t));
  DISASS(" %u", rcount);
  if (rcount > 1) ERR("!expected result count <= 1, got %d", rcount);
  for (uint32_t i = 0; i < rcount && (buf->ptr < buf->end); i++) {
    wasm_type_t t = read_value_type(buf);
    dest->results[i] = t;
  }
  DISASS("\n");
}

uint8_t bind_import(wasm_module_t* module, wasm_import_decl_t* decl) {
  if (strcmp(decl->mod_name, "weewasm") != 0) ERR("!unrecognized import module: %s", decl->mod_name);
  if (strcmp(decl->member_name, "puti") == 0) return WEEWASM_INTRINSIC_PUTI; // TODO: check signature
  if (strcmp(decl->member_name, "putd") == 0) return WEEWASM_INTRINSIC_PUTD;
  if (strcmp(decl->member_name, "puts") == 0) return WEEWASM_INTRINSIC_PUTS;
  ERR("!unrecognized weewasm import: %s", decl->member_name);
  return 0;
}

void read_import_decl(buffer_t* buf, wasm_module_t* module, uint32_t import_index, uint32_t func_index, const byte* sectend) {
  DISASS(" ");
  wasm_import_decl_t* dest = &module->imports[import_index];
  read_and_copy_string(buf, &dest->mod_name, sectend);
  DISASS(" ");
  read_and_copy_string(buf, &dest->member_name, sectend);
  uint32_t code = read_u8(buf);
  DISASS(" %s", import_kind_name(code));
  switch (code) {
  case WASM_IMPORT_FUNC: {
    uint32_t sig = read_u32leb(buf);
    DISASS(" %u", sig);
    dest->kind = FUNC;
    dest->index = func_index;
    wasm_func_decl_t* func = &module->funcs[func_index];
    func->sig_index = sig;
    func->intrinsic = bind_import(module, dest);
    break;
  }
  case WASM_IMPORT_TABLE: {
    ERR("!illegal table import");
    break;
  }
  case WASM_IMPORT_MEMORY: {
    ERR("!illegal memory import");
    break;
  }
  case WASM_IMPORT_GLOBAL: {
    ERR("!illegal global import");
    break;
  }
  }
  DISASS("\n");
}

void read_func_decl(buffer_t* buf, wasm_func_decl_t* dest, const byte* sectend) {
  uint32_t index = read_u32leb(buf);
  dest->sig_index = index;
  DISASS(" %u \t\t\t; signature index\n", index);
}

void read_table_decl(buffer_t* buf, wasm_table_decl_t* dest, const byte* sectend) {
  int32_t type = read_i32leb(buf);
  DISASS(" %s", type_name(type));
  if (type != WASM_TYPE_FUNCREF) ERR("!invalid table type %s", type_name(type));
  read_limits(buf, &dest->limits);
  DISASS("\n");
}

void read_memory_decl(buffer_t* buf, wasm_limits_t* limits, const byte* sectend) {
  read_limits(buf, limits);
  DISASS("\n");
}

void read_global_decl(buffer_t* buf, wasm_global_decl_t* dest, const byte* sectend) {
  dest->type = read_value_type(buf);
  byte mut = read_u8(buf);
  dest->mutable = mut != 0;
  if (mut != 0) DISASS(" mutable");
  DISASS("\n");
  dest->init = read_init_expr(buf);
}

void read_export_decl(buffer_t* buf, wasm_module_t* module, const byte* sectend) {
  DISASS(" ");
  uint32_t namelen = 0;
  const char* name = NULL;
  read_and_copy_string(buf, &name, sectend);
  if (strcmp(name, "main") != 0) ERR("!expected export name \"main\", got %s", name);
  free((char*)name); // XXX: avoid copy?

  uint32_t code = read_u8(buf);
  DISASS(" %s", import_kind_name(code));
  uint32_t index = read_u32leb(buf);
  DISASS(" %u \t\t\t; export index\n", index);
  switch (code) {
  case WASM_IMPORT_FUNC: {
    module->main_func = (int32_t)index;
    break;
  }
  case WASM_IMPORT_TABLE: {
    ERR("!illegal table export");
    break;
  }
  case WASM_IMPORT_MEMORY: {
    ERR("!illegal memory export");
    break;
  }
  case WASM_IMPORT_GLOBAL: {
    ERR("!illegal global export");
    break;
  }
  }
}

uint32_t read_offset_expr(buffer_t* buf) {
  uint32_t val = 0;
  while (buf->ptr < buf->end) {
    byte b = read_u8(buf);
    if (b == WASM_OP_I32_CONST) {
      val = (uint32_t)read_i32leb(buf);
      DISASS(" %u \t\t\t; offset\n", val);
    } else if (b == WASM_OP_END) {
      break;
    } else {
      DISASS("<!unexpected bytecode %02X>", b);
      break;
    }
  }
  return val;
}

void read_elems_decl(buffer_t* buf, wasm_elems_decl_t* dest, const byte* sectend) {
  byte b = read_u32leb(buf);
  if (b != 0) {
    DISASS(" <!invalid elem flags %d>", b);
  } else {
    DISASS("  ");
    dest->table_offset = read_offset_expr(buf);
    uint32_t count = read_u32leb(buf);
    dest->length = count;
    dest->func_indexes = (uint32_t*)malloc(dest->length * sizeof(uint32_t));
    DISASS("   %u \t\t\t; count\n    ", count);
    for (uint32_t i = 0; i < count && buf->ptr < buf->end; i++) {
      uint32_t f = read_u32leb(buf);
      DISASS(" %u", f);
      dest->func_indexes[i] = f;
    }
  }
  DISASS("\n");
}

void read_code_decl(buffer_t* buf, wasm_func_decl_t* dest, const byte* sectend) {
  uint32_t len = read_u32leb(buf);
  dest->code_start = (uint32_t)(buf->ptr - buf->start);
  const byte* codeend = buf->ptr + len;
  DISASS("body %u \t\t\t; length\n", len);
  uint32_t cnt = read_u32leb(buf);
  DISASS(" %u \t\t\t; locals count\n", cnt);
  for (uint32_t i = 0; i < cnt; i++) {
    uint32_t cnt = read_u32leb(buf);
    DISASS("  %u", cnt);
    int32_t type = read_i32leb(buf);
    DISASS(" %s\n", type_name(type));
  }
  if (g_disassemble) {
    // disassemble code
    while (buf->ptr < codeend && buf->ptr < buf->end) {
      print_bytecode(buf);
    }
  }
  buf->ptr = codeend;
  dest->code_end = (uint32_t)(buf->ptr - buf->start);
}

void read_data_decl(buffer_t* buf, wasm_data_decl_t* dest, const byte* sectend) {
  byte b = read_u32leb(buf);
  if (b != 0) {
    DISASS(" <!invalid data flags %d>", b);
  } else {
    dest->mem_offset = read_offset_expr(buf);
    uint32_t count = read_u32leb(buf);
    DISASS("   %u \t\t\t; bytes count\n", count);
    dest->bytes_start = (uint32_t)(buf->ptr - buf->start);
    if (g_disassemble) {
      print_data(buf, count);
    } else {
      buf->ptr += count;
      if (buf->ptr > buf->end) {
        ERR("!too much data, %d bytes", count);
        buf->ptr = buf->end;
      }
    }
    dest->bytes_end = (uint32_t)(buf->ptr - buf->start);
  }
  DISASS("\n");
}

void allocMoreItems(void** ptr, uint32_t* nptr, size_t item_size, uint32_t add) {
  byte* curbuf = *ptr;
  uint32_t curcnt = *nptr;
  size_t cursize = curcnt * item_size;
  uint32_t newcnt = curcnt + add;
  size_t newsize = newcnt * item_size;
  byte* newbuf = (byte*)realloc(curbuf, newsize);
  memset(newbuf + cursize, 0, newsize - cursize);
  *ptr = newbuf;
  *nptr = newcnt;
}

uint32_t read_count(buffer_t* buf, uint32_t max, byte sectcode) {
  uint32_t count = read_u32leb(buf);
  DISASS("%u \t\t\t; count\n", count);
  if (count > max) ERR("!expected count <= %d for %s section, got %d", max, section_name(sectcode), count);
  return count;
}

// A macro to read the count of entries, expand internal storage, and call {read_func} in a loop.
#define READ_ENTRIES(count_field, ptr_field, entry_type, read_func) do { \
    uint32_t count = read_count(buf, 100000, sectcode);                \
    uint32_t j = module->count_field;                                   \
    allocMoreItems((void**)&module->ptr_field, &module->count_field, sizeof(entry_type), count); \
    for (uint32_t i = 0; i < count; i++, j++) {                         \
      read_func(buf, &module->ptr_field[j], sectend);                   \
    }                                                                   \
  } while (0)


// The main parsing routine.
int parse_wasm_module(buffer_t* buf, wasm_module_t* module) {
  ssize_t len = 0;

  //==== Check magic word =============================================
  uint32_t magic = decode_u32(buf->ptr, buf->end, &len);
  DISASS("read_magic = %x\n", magic);
  if (len != 4 || magic != WASM_MAGIC) {
    ERR("!invalid magic word, expected %x\n", WASM_MAGIC);
    return -1;
  }
  buf->ptr += 4;

  //==== Check Wasm Version ===========================================
  uint32_t version = decode_u32(buf->ptr, buf->end, &len);
  DISASS("read_version = %x\n", version);
  if (len != 4 || version != WASM_VERSION) {
    ERR("!invalid Wasm version, expected %x\n", WASM_VERSION);
    return -2;
  }
  buf->ptr += 4;

  //==== Read Wasm sections ===========================================
  while (buf->ptr < buf->end) {
    uint32_t sectlen = 0;
    int sectcode = read_section_code(buf, module, &sectlen);
    if (sectcode < 0) return -3;
    const byte* sectend = buf->ptr + sectlen;

    switch (sectcode) {
    case WASM_SECT_TYPE: {
      READ_ENTRIES(num_sigs, sigs, wasm_sig_decl_t, read_type_decl);
      break;
    }
    case WASM_SECT_IMPORT: {
      uint32_t count = read_count(buf, 1000, sectcode);
      uint32_t import_index = module->num_imports;
      uint32_t func_index = module->num_funcs;
      // all imports must be functions.
      allocMoreItems((void**)&module->imports, &module->num_imports, sizeof(wasm_import_decl_t), count);
      allocMoreItems((void**)&module->funcs, &module->num_funcs, sizeof(wasm_func_decl_t), count);
      for (uint32_t i = 0; i < count; i++, import_index++, func_index++) {
        read_import_decl(buf, module, import_index, func_index, sectend);
      }
      break;
    }
    case WASM_SECT_FUNCTION: {
      READ_ENTRIES(num_funcs, funcs, wasm_func_decl_t, read_func_decl);
      break;
    }
    case WASM_SECT_TABLE: {
      read_count(buf, 1, sectcode);
      if (module->table != NULL) ERR("!repeated table section");
      module->table = (wasm_table_decl_t*)malloc(sizeof(wasm_table_decl_t));
      read_table_decl(buf, module->table, sectend);
      break;
    }
    case WASM_SECT_MEMORY: {
      read_count(buf, 1, sectcode);
      read_memory_decl(buf, &module->mem_limits, sectend);
      break;
    }
    case WASM_SECT_GLOBAL: {
      READ_ENTRIES(num_globals, globals, wasm_global_decl_t, read_global_decl);
      break;
    }
    case WASM_SECT_EXPORT: {
      read_count(buf, 1, sectcode);
      read_export_decl(buf, module, sectend);
      break;
    }
    case WASM_SECT_START: {
      uint32_t entry = read_u32leb(buf);
      module->start_func = (int32_t)entry;
      DISASS("%u\n", entry);
      break;
    }
    case WASM_SECT_ELEMENT: {
      READ_ENTRIES(num_elems, elems, wasm_elems_decl_t, read_elems_decl);
      break;
    }
    case WASM_SECT_CODE: {
      uint32_t num_bodies = module->num_funcs - module->num_imports;
      uint32_t count = read_count(buf, num_bodies, sectcode);
      if (count != num_bodies) ERR("!expected %d function bodies, got %d", num_bodies, count);
      uint32_t func_index = module->num_imports;
      for (uint32_t i = 0; i < count; i++, func_index++) {
        read_code_decl(buf, &module->funcs[func_index], sectend);
      }
      break;
    }
    case WASM_SECT_DATA: {
      READ_ENTRIES(num_data, data, wasm_data_decl_t, read_data_decl);
      break;
    }
    default:
      ERR("!unknown section constant 0x%02x\n", sectcode);
      return -4;
    }
    CHECK(buf->ptr == sectend); // internal check
  }
  return 0;
}

