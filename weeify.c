#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <stddef.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "vm.h"
#include "test.h"
#include "weewasm.h"
#include "illegal.h"

int weeify(int out_fd, const byte* start, const byte* end);

// Parse arguments and {weeify()} a single file.
int main(int argc, char* argv[]) {
  const char* in_path = NULL;
  const char* out_path = NULL;

  // parse arguments
  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];
    if (strcmp(arg, "-trace") == 0) {
      g_trace = 1;
      continue;
    }
    if (strcmp(arg, "-o") == 0 && i < (argc - 1)) {
      out_path = argv[++i];
      continue;
    }
    if (in_path != NULL) {
      out_path = NULL;
      break;
    }
    in_path = arg;
  }

  // check both {in_path} and {out_path} are specified
  if (in_path == NULL || out_path == NULL) {
    printf("Usage: weeify [-trace] -o <out_file> <in_file>\n");
    return 1;
  }

  // load input file and create output file
  {
    byte* start = NULL;
    byte* end = NULL;
    ssize_t r = load_file(in_path, &start, &end);
    if (r < 0) {
      ERR("failed to load: %s\n", in_path);
      return 2;
    }
    TRACE("loaded %s: %ld bytes\n", in_path, r);

    int out_fd = open(out_path, O_RDWR | O_CREAT, 0644);
    if (out_fd < 0) {
      ERR("failed to create: %s\n", out_path);
      return 3;
    }
    weeify(out_fd, start, end);
    close(out_fd);
    unload_file(&start, &end);
    return 0;
  }
}

// emits a single byte
void emit1(int out_fd, byte val) {
  write(out_fd, &val, 1);
}

// emits an unsigned LEB, not padded
void emit_u32leb(int out_fd, uint32_t val) {
  do {
    uint32_t next = val >> 7;
    uint32_t v = val & 0x7F;
    if (next != 0) v |= 0x80;
    emit1(out_fd, v);
    val = next;
  } while (val != 0);
}

// emits an unsigned LEB, padded to 4 bytes
void emit_u32leb4(int out_fd, uint32_t val) {
  byte buf[4];
  buf[0] = 0x80 | (val & 0x7F);
  buf[1] = 0x80 | ((val >> 7) & 0x7F);
  buf[2] = 0x80 | ((val >> 14) & 0x7F);
  buf[3] =        ((val >> 21) & 0x7F);
  write(out_fd, buf, 4);
}

// emits an unsigned LEB, padded to 5 bytes
void emit_u32leb5(int out_fd, uint32_t val) {
  byte buf[5];
  buf[0] = 0x80 | (val & 0x7F);
  buf[1] = 0x80 | ((val >> 7) & 0x7F);
  buf[2] = 0x80 | ((val >> 14) & 0x7F);
  buf[3] = 0x80 | ((val >> 21) & 0x7F);
  buf[4] =        ((val >> 28) & 0x7F);
  write(out_fd, buf, 5);
}

// emits the bytes from {start ... buf->ptr}
void emit_from(int out_fd, const byte* start, buffer_t* buf) {
  write(out_fd, start, (buf->ptr - start));
}

off_t emit_reserved_length(int out_fd) {
  off_t before = lseek(out_fd, 0, SEEK_CUR);
  TRACE("  reserve 5 byte LEB for length @ %lld\n", (long long)before);
  emit_u32leb5(out_fd, 0);
  return before;
}

void emit_patched_length(int out_fd, off_t before) {
  off_t cur = lseek(out_fd, 0, SEEK_CUR);
  off_t diff = cur - before - 5;
  TRACE("  patch 5 byte LEB for length @ %lld = %lld\n", (long long)before, (long long)diff);
  lseek(out_fd, before, SEEK_SET);
  emit_u32leb5(out_fd, diff);
  lseek(out_fd, cur, SEEK_SET);
}

// Returns the string name of a section code.
const char* section_name(byte code) {
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
  case 0: return "custom";
  default:
    return "unknown";
  }
}

#define WASM_TYPE_I32 -1
#define WASM_TYPE_I64 -2
#define WASM_TYPE_F32 -3
#define WASM_TYPE_F64 -4
#define WASM_TYPE_V128 -5
#define WASM_TYPE_EXTERNREF -17
#define WASM_TYPE_FUNCREF -16

// Returns the string name of a type code.
const char* type_name(uint32_t code) {
  switch (code) {
  case WASM_TYPE_I32: return "i32";
  case WASM_TYPE_I64: return "<!illegal i64>";
  case WASM_TYPE_F32: return "<!illegal f32>";
  case WASM_TYPE_F64: return "f64";
  case WASM_TYPE_V128: return "<!illegal v128>";
  case WASM_TYPE_EXTERNREF: return "externref";
  case WASM_TYPE_FUNCREF: return "funcref";
  default: return "<!unknown type>";
  }
}

int transform_code_section(int out_fd, buffer_t* buf);
int transform_body(int out_fd, buffer_t* buf);

// Transforms the wasm file in {start ... end} and outputs the bytes to {out_fd}.
int weeify(int out_fd, const byte* start, const byte* end) {
  buffer_t onstack_buf = {
    start,
    start,
    end
  };
  buffer_t* buf = &onstack_buf;
  ssize_t len = 0;

  //==== Check magic word =============================================
  uint32_t magic = decode_u32(buf->ptr, buf->end, &len);
  TRACE("read_magic = 0x%08x\n", magic);
  if (len != 4 || magic != WASM_MAGIC) {
    ERR("invalid magic word, expected %x\n", WASM_MAGIC);
    return -1;
  }
  buf->ptr += 4;
  
  //==== Check Wasm Version ===========================================
  uint32_t version = decode_u32(buf->ptr, buf->end, &len);
  TRACE("read_version = 0x%08x\n", version);
  if (len != 4 || version != WASM_VERSION) {
    ERR("invalid Wasm version, expected %x\n", WASM_VERSION);
    return -2;
  }
  buf->ptr += 4;

  TRACE("  copy header\n");
  emit_from(out_fd, buf->start, buf);
  
  //==== Read Wasm sections ===========================================
  while (buf->ptr < buf->end) {
    const byte* section_start = buf->ptr;
    
    byte code = read_u8(buf);
    uint32_t sectlen = read_u32leb(buf);
    TRACE("read_section_code = 0x%02x (%s), %u bytes\n", code, section_name(code), sectlen);
    const byte* section_end = buf->ptr + sectlen;
    if (section_end > end) {
      ERR("invalid: section length too large");
      return -4;
    }
    
    if (code == WASM_SECT_CODE) {
      // transform a code section
      buffer_t code_buf = { // use a sub-buffer to avoid going OOB
	section_start,
	buf->ptr,
	section_end
      };
      int result = transform_code_section(out_fd, &code_buf);
      if (result != 0) return result;
    } else {
      // not a code section; copy section verbatim
      ptrdiff_t diff = (section_end - section_start);
      TRACE("  copy %td byte section\n", diff);
      write(out_fd, section_start, diff);
    }

    buf->ptr = section_end;

    if (section_end == section_start) {
      ERR("internal error: zero-length section");
      return -3;
    }
  }
  return 0;
}

// Transforms a code section, padding branch instructions to be at least 4-byte LEBs.
int transform_code_section(int out_fd, buffer_t* buf) {
  emit1(out_fd, WASM_SECT_CODE);
  off_t before = emit_reserved_length(out_fd);
  
  uint32_t body_count = read_u32leb(buf);
  emit_u32leb(out_fd, body_count);
  for (uint32_t i = 0; i < body_count; i++) {
    TRACE("transform body #%u\n", i);
    const byte* body_start = buf->ptr;
    uint32_t body_len = read_u32leb(buf);
    buffer_t body_buf = { // use a sub-buffer to avoid going OOB
      body_start,
      buf->ptr,
      buf->ptr + body_len
    };

    transform_body(out_fd, &body_buf);
    buf->ptr = body_buf.end;
  }
  emit_patched_length(out_fd, before);
  return 0;
}

// Support for disassembling/transforming the bytecode
typedef struct {
  const char* mnemonic;
  int immkind;
  int illegal;
} opcode_imm_t;

#define IMM_NONE 0
#define IMM_BLOCKT 1
#define IMM_LABEL 2
#define IMM_LABELS 3
#define IMM_FUNC 4
#define IMM_SIG_TABLE 5
#define IMM_LOCAL 6
#define IMM_GLOBAL 7
#define IMM_TABLE 8
#define IMM_MEMARG 9
#define IMM_I32 10
#define IMM_F64 11
#define IMM_MEMORY 12
#define IMM_TAG 13
#define IMM_I64 14
#define IMM_F32 15
#define IMM_REFNULLT 16
#define IMM_VALTS 17

opcode_imm_t opcode_imm_table[256] = {
  [WASM_OP_UNREACHABLE]		= {"unreachable" },
  [WASM_OP_NOP]			= {"nop" },
  [WASM_OP_BLOCK]		= {"block", IMM_BLOCKT },
  [WASM_OP_LOOP]		= {"loop", IMM_BLOCKT },
  [WASM_OP_END]			= {"end" },
  [WASM_OP_BR]			= {"br", IMM_LABEL },
  [WASM_OP_BR_IF]		= {"br_if", IMM_LABEL },
  [WASM_OP_BR_TABLE]		= {"br_table", IMM_LABELS },
  [WASM_OP_RETURN]		= {"return" },
  [WASM_OP_CALL]		= {"call", IMM_FUNC },
  [WASM_OP_CALL_INDIRECT]	= {"call_indirect", IMM_SIG_TABLE },
  [WASM_OP_DROP]		= {"drop" },
  [WASM_OP_SELECT]		= {"select" },
  [WASM_OP_LOCAL_GET]		= {"local.get", IMM_LOCAL },
  [WASM_OP_LOCAL_SET]		= {"local.set", IMM_LOCAL },
  [WASM_OP_LOCAL_TEE]		= {"local.tee", IMM_LOCAL },
  [WASM_OP_GLOBAL_GET]		= {"global.get", IMM_GLOBAL },
  [WASM_OP_GLOBAL_SET]		= {"global.set", IMM_GLOBAL },
  [WASM_OP_TABLE_GET]		= {"table.get", IMM_TABLE },
  [WASM_OP_TABLE_SET]		= {"table.set", IMM_TABLE },
  [WASM_OP_I32_LOAD]		= {"i32.load", IMM_MEMARG },
  [WASM_OP_F64_LOAD]		= {"f64.load", IMM_MEMARG },
  [WASM_OP_I32_LOAD8_S]		= {"i32.load8_s", IMM_MEMARG },
  [WASM_OP_I32_LOAD8_U]		= {"i32.load8_u", IMM_MEMARG },
  [WASM_OP_I32_LOAD16_S]	= {"i32.load16_s", IMM_MEMARG },
  [WASM_OP_I32_LOAD16_U]	= {"i32.load16_u", IMM_MEMARG },
  [WASM_OP_I32_STORE]		= {"i32.store", IMM_MEMARG },
  [WASM_OP_F64_STORE]		= {"f64.store", IMM_MEMARG },
  [WASM_OP_I32_STORE8]		= {"i32.store8", IMM_MEMARG },
  [WASM_OP_I32_STORE16]		= {"i32.store16", IMM_MEMARG },
  [WASM_OP_I32_CONST]		= {"i32.const", IMM_I32 },
  [WASM_OP_F64_CONST]		= {"f64.const", IMM_F64 },
  [WASM_OP_I32_EQZ]		= {"i32.eqz" },
  [WASM_OP_I32_EQ]		= {"i32.eq" },
  [WASM_OP_I32_NE]		= {"i32.ne" },
  [WASM_OP_I32_LT_S]		= {"i32.lt_s" },
  [WASM_OP_I32_LT_U]		= {"i32.lt_u" },
  [WASM_OP_I32_GT_S]		= {"i32.gt_s" },
  [WASM_OP_I32_GT_U]		= {"i32.gt_u" },
  [WASM_OP_I32_LE_S]		= {"i32.le_s" },
  [WASM_OP_I32_LE_U]		= {"i32.le_u" },
  [WASM_OP_I32_GE_S]		= {"i32.ge_s" },
  [WASM_OP_I32_GE_U]		= {"i32.ge_u" },
  [WASM_OP_F64_EQ]		= {"f64.eq" },
  [WASM_OP_F64_NE]		= {"f64.ne" },
  [WASM_OP_F64_LT]		= {"f64.lt" },
  [WASM_OP_F64_GT]		= {"f64.gt" },
  [WASM_OP_F64_LE]		= {"f64.le" },
  [WASM_OP_F64_GE]		= {"f64.ge" },
  [WASM_OP_I32_CLZ]		= {"i32.clz" },
  [WASM_OP_I32_CTZ]		= {"i32.ctz" },
  [WASM_OP_I32_POPCNT]		= {"i32.popcnt" },
  [WASM_OP_I32_ADD]		= {"i32.add" },
  [WASM_OP_I32_SUB]		= {"i32.sub" },
  [WASM_OP_I32_MUL]		= {"i32.mul" },
  [WASM_OP_I32_DIV_S]		= {"i32.div_s" },
  [WASM_OP_I32_DIV_U]		= {"i32.div_u" },
  [WASM_OP_I32_REM_S]		= {"i32.rem_s" },
  [WASM_OP_I32_REM_U]		= {"i32.rem_u" },
  [WASM_OP_I32_AND]		= {"i32.and" },
  [WASM_OP_I32_OR]		= {"i32.or" },
  [WASM_OP_I32_XOR]		= {"i32.xor" },
  [WASM_OP_I32_SHL]		= {"i32.shl" },
  [WASM_OP_I32_SHR_S]		= {"i32.shr_s" },
  [WASM_OP_I32_SHR_U]		= {"i32.shr_u" },
  [WASM_OP_I32_ROTL]		= {"i32.rotl" },
  [WASM_OP_I32_ROTR]		= {"i32.rotr" },
  [WASM_OP_F64_ADD]		= {"f64.add" },
  [WASM_OP_F64_SUB]		= {"f64.sub" },
  [WASM_OP_F64_MUL]		= {"f64.mul" },
  [WASM_OP_F64_DIV]		= {"f64.div" },
  [WASM_OP_I32_TRUNC_F64_S]	= {"i32.trunc_f64_s" },
  [WASM_OP_I32_TRUNC_F64_U]	= {"i32.trunc_f64_u" },
  [WASM_OP_F64_CONVERT_I32_S]	= {"f64.convert_i32_s" },
  [WASM_OP_F64_CONVERT_I32_U]	= {"f64.convert_i32_u" },
  [WASM_OP_F64_CONVERT_I64_S]	= {"f64.convert_i64_s" },
  [WASM_OP_F64_CONVERT_I64_U]	= {"f64.convert_i64_u" },
  [WASM_OP_I32_EXTEND8_S]	= {"i32.extend8_s" },
  [WASM_OP_I32_EXTEND16_S]	= {"i32.extend16_s" },
  // illegal bytecodes
  [WASM_OP_IF]			= {"if", IMM_BLOCKT, -1 },
  [WASM_OP_ELSE]		= {"else", -1 },
  [WASM_OP_TRY]			= {"try", IMM_BLOCKT, -1},
  [WASM_OP_CATCH]		= {"catch", IMM_TAG, -1},
  [WASM_OP_THROW]		= {"throw", IMM_TAG, -1},
  [WASM_OP_RETHROW]		= {"rethrow", IMM_NONE, -1},
  [WASM_OP_RETURN_CALL]		= {"return_call", IMM_FUNC, -1},
  [WASM_OP_RETURN_CALL_INDIRECT]= {"return_call_indirect", IMM_SIG_TABLE, -1},
  [WASM_OP_CALL_REF]		= {"call_ref", IMM_NONE, -1},
  [WASM_OP_RETURN_CALL_REF]	= {"return_call_ref", IMM_NONE, -1},
  [WASM_OP_DELEGATE]		= {"delegate", IMM_NONE, -1},
  [WASM_OP_CATCH_ALL]		= {"catch_all", IMM_NONE, -1},
  [WASM_OP_SELECT_T]		= {"select", IMM_VALTS, -1},
  [WASM_OP_I64_LOAD]		= {"i64.load", IMM_MEMARG, -1},
  [WASM_OP_F32_LOAD]		= {"f32.load", IMM_MEMARG, -1},
  [WASM_OP_I64_LOAD8_S]		= {"i64.load8_s", IMM_MEMARG, -1},
  [WASM_OP_I64_LOAD8_U]		= {"i64.load8_u", IMM_MEMARG, -1},
  [WASM_OP_I64_LOAD16_S]	= {"i64.load16_s", IMM_MEMARG, -1},
  [WASM_OP_I64_LOAD16_U]	= {"i64.load16_u", IMM_MEMARG, -1},
  [WASM_OP_I64_LOAD32_S]	= {"i64.load32_s", IMM_MEMARG, -1},
  [WASM_OP_I64_LOAD32_U]	= {"i64.load32_u", IMM_MEMARG, -1},
  [WASM_OP_I64_STORE]		= {"i64.store", IMM_MEMARG, -1},
  [WASM_OP_F32_STORE]		= {"f32.store", IMM_MEMARG, -1},
  [WASM_OP_I64_STORE8]		= {"i64.store8", IMM_MEMARG, -1},
  [WASM_OP_I64_STORE16]		= {"i64.store16", IMM_MEMARG, -1},
  [WASM_OP_I64_STORE32]		= {"i64.store32", IMM_MEMARG, -1},
  [WASM_OP_MEMORY_SIZE]		= {"memory.size", IMM_MEMORY, -1},
  [WASM_OP_MEMORY_GROW]		= {"memory.grow", IMM_MEMORY, -1},
  [WASM_OP_I64_CONST]		= {"i64.const", IMM_I64, -1},
  [WASM_OP_F32_CONST]		= {"f32.const", IMM_F32, -1},
  [WASM_OP_I64_EQZ]		= {"i64.eqz", IMM_NONE, -1},
  [WASM_OP_I64_EQ]		= {"i64.eq", IMM_NONE, -1},
  [WASM_OP_I64_NE]		= {"i64.ne", IMM_NONE, -1},
  [WASM_OP_I64_LT_S]		= {"i64.lt_s", IMM_NONE, -1},
  [WASM_OP_I64_LT_U]		= {"i64.lt_u", IMM_NONE, -1},
  [WASM_OP_I64_GT_S]		= {"i64.gt_s", IMM_NONE, -1},
  [WASM_OP_I64_GT_U]		= {"i64.gt_u", IMM_NONE, -1},
  [WASM_OP_I64_LE_S]		= {"i64.le_s", IMM_NONE, -1},
  [WASM_OP_I64_LE_U]		= {"i64.le_u", IMM_NONE, -1},
  [WASM_OP_I64_GE_S]		= {"i64.ge_s", IMM_NONE, -1},
  [WASM_OP_I64_GE_U]		= {"i64.ge_u", IMM_NONE, -1},
  [WASM_OP_F32_EQ]		= {"f32.eq", IMM_NONE, -1},
  [WASM_OP_F32_NE]		= {"f32.ne", IMM_NONE, -1},
  [WASM_OP_F32_LT]		= {"f32.lt", IMM_NONE, -1},
  [WASM_OP_F32_GT]		= {"f32.gt", IMM_NONE, -1},
  [WASM_OP_F32_LE]		= {"f32.le", IMM_NONE, -1},
  [WASM_OP_F32_GE]		= {"f32.ge", IMM_NONE, -1},
  [WASM_OP_I64_CLZ]		= {"i64.clz", IMM_NONE, -1},
  [WASM_OP_I64_CTZ]		= {"i64.ctz", IMM_NONE, -1},
  [WASM_OP_I64_POPCNT]		= {"i64.popcnt", IMM_NONE, -1},
  [WASM_OP_I64_ADD]		= {"i64.add", IMM_NONE, -1},
  [WASM_OP_I64_SUB]		= {"i64.sub", IMM_NONE, -1},
  [WASM_OP_I64_MUL]		= {"i64.mul", IMM_NONE, -1},
  [WASM_OP_I64_DIV_S]		= {"i64.div_s", IMM_NONE, -1},
  [WASM_OP_I64_DIV_U]		= {"i64.div_u", IMM_NONE, -1},
  [WASM_OP_I64_REM_S]		= {"i64.rem_s", IMM_NONE, -1},
  [WASM_OP_I64_REM_U]		= {"i64.rem_u", IMM_NONE, -1},
  [WASM_OP_I64_AND]		= {"i64.and", IMM_NONE, -1},
  [WASM_OP_I64_OR]		= {"i64.or", IMM_NONE, -1},
  [WASM_OP_I64_XOR]		= {"i64.xor", IMM_NONE, -1},
  [WASM_OP_I64_SHL]		= {"i64.shl", IMM_NONE, -1},
  [WASM_OP_I64_SHR_S]		= {"i64.shr_s", IMM_NONE, -1},
  [WASM_OP_I64_SHR_U]		= {"i64.shr_u", IMM_NONE, -1},
  [WASM_OP_I64_ROTL]		= {"i64.rotl", IMM_NONE, -1},
  [WASM_OP_I64_ROTR]		= {"i64.rotr", IMM_NONE, -1},
  [WASM_OP_F32_ABS]		= {"f32.abs", IMM_NONE, -1},
  [WASM_OP_F32_NEG]		= {"f32.neg", IMM_NONE, -1},
  [WASM_OP_F32_CEIL]		= {"f32.ceil", IMM_NONE, -1},
  [WASM_OP_F32_FLOOR]		= {"f32.floor", IMM_NONE, -1},
  [WASM_OP_F32_TRUNC]		= {"f32.trunc", IMM_NONE, -1},
  [WASM_OP_F32_NEAREST]		= {"f32.nearest", IMM_NONE, -1},
  [WASM_OP_F32_SQRT]		= {"f32.sqrt", IMM_NONE, -1},
  [WASM_OP_F32_ADD]		= {"f32.add", IMM_NONE, -1},
  [WASM_OP_F32_SUB]		= {"f32.sub", IMM_NONE, -1},
  [WASM_OP_F32_MUL]		= {"f32.mul", IMM_NONE, -1},
  [WASM_OP_F32_DIV]		= {"f32.div", IMM_NONE, -1},
  [WASM_OP_F32_MIN]		= {"f32.min", IMM_NONE, -1},
  [WASM_OP_F32_MAX]		= {"f32.max", IMM_NONE, -1},
  [WASM_OP_F32_COPYSIGN]	= {"f32.copysign", IMM_NONE, -1},
  [WASM_OP_F64_ABS]		= {"f64.abs", IMM_NONE, -1},
  [WASM_OP_F64_NEG]		= {"f64.neg", IMM_NONE, -1},
  [WASM_OP_F64_CEIL]		= {"f64.ceil", IMM_NONE, -1},
  [WASM_OP_F64_FLOOR]		= {"f64.floor", IMM_NONE, -1},
  [WASM_OP_F64_TRUNC]		= {"f64.trunc", IMM_NONE, -1},
  [WASM_OP_F64_NEAREST]		= {"f64.nearest", IMM_NONE, -1},
  [WASM_OP_F64_SQRT]		= {"f64.sqrt", IMM_NONE, -1},
  [WASM_OP_F64_MIN]		= {"f64.min", IMM_NONE, -1},
  [WASM_OP_F64_MAX]		= {"f64.max", IMM_NONE, -1},
  [WASM_OP_F64_COPYSIGN]	= {"f64.copysign", IMM_NONE, -1},
  [WASM_OP_I32_WRAP_I64]	= {"i32.wrap_i64", IMM_NONE, -1},
  [WASM_OP_I32_TRUNC_F32_S]	= {"i32.trunc_f32_s", IMM_NONE, -1},
  [WASM_OP_I32_TRUNC_F32_U]	= {"i32.trunc_f32_u", IMM_NONE, -1},
  [WASM_OP_I64_EXTEND_I32_S]	= {"i64.extend_i32_s", IMM_NONE, -1},
  [WASM_OP_I64_EXTEND_I32_U]	= {"i64.extend_i32_u", IMM_NONE, -1},
  [WASM_OP_I64_TRUNC_F32_S]	= {"i64.trunc_f32_s", IMM_NONE, -1},
  [WASM_OP_I64_TRUNC_F32_U]	= {"i64.trunc_f32_u", IMM_NONE, -1},
  [WASM_OP_I64_TRUNC_F64_S]	= {"i64.trunc_f64_s", IMM_NONE, -1},
  [WASM_OP_I64_TRUNC_F64_U]	= {"i64.trunc_f64_u", IMM_NONE, -1},
  [WASM_OP_F32_CONVERT_I32_S]	= {"f32.convert_i32_s", IMM_NONE, -1},
  [WASM_OP_F32_CONVERT_I32_U]	= {"f32.convert_i32_u", IMM_NONE, -1},
  [WASM_OP_F32_CONVERT_I64_S]	= {"f32.convert_i64_s", IMM_NONE, -1},
  [WASM_OP_F32_CONVERT_I64_U]	= {"f32.convert_i64_u", IMM_NONE, -1},
  [WASM_OP_F32_DEMOTE_F64]	= {"f32.demote_f64", IMM_NONE, -1},
  [WASM_OP_F64_PROMOTE_F32]	= {"f64.promote_f32", IMM_NONE, -1},
  [WASM_OP_I32_REINTERPRET_F32]	= {"i32.reinterpret_f32", IMM_NONE, -1},
  [WASM_OP_I64_REINTERPRET_F64]	= {"i64.reinterpret_f64", IMM_NONE, -1},
  [WASM_OP_F32_REINTERPRET_I32]	= {"f32.reinterpret_i32", IMM_NONE, -1},
  [WASM_OP_F64_REINTERPRET_I64]	= {"f64.reinterpret_i64", IMM_NONE, -1},
  [WASM_OP_I64_EXTEND8_S]	= {"i64.extend8_s", IMM_NONE, -1},
  [WASM_OP_I64_EXTEND16_S]	= {"i64.extend16_s", IMM_NONE, -1},
  [WASM_OP_I64_EXTEND32_S]	= {"i64.extend32_s", IMM_NONE, -1},
  [WASM_OP_REF_NULL]		= {"ref.null", IMM_REFNULLT, -1},
  [WASM_OP_REF_IS_NULL]		= {"ref.is_null", IMM_NONE, -1},
  [WASM_OP_REF_FUNC]		= {"ref.func", IMM_FUNC, -1},
  [WASM_OP_REF_AS_NON_NULL]	= {"ref.as_non_null", IMM_NONE, -1},
  [WASM_OP_BR_ON_NULL]		= {"br_on_null", IMM_LABEL, -1},
  [WASM_OP_REF_EQ]		= {"ref.eq", IMM_NONE, -1},
  [WASM_OP_BR_ON_NON_NULL]	= {"br_on_non_null", IMM_LABEL, -1},
};

int transform_bytecode(int out_fd, buffer_t* buf) {
  TRACE("  ");
  const byte* start = buf->ptr;
  
  byte code = read_u8(buf);
  opcode_imm_t* entry = &opcode_imm_table[code];
  if (entry->mnemonic == NULL) {
    ERR("<!illegal bytecode %02X>", code);
    return -3;
  } else if (entry->illegal) {
    ERR("<!illegal %s>", entry->mnemonic);
    return -4;
  } else {
    TRACE("%s", entry->mnemonic);
  }
  switch (entry->immkind) {
  case IMM_LABEL: {
    // expand to 4-byte LEB
    uint32_t imm = read_u32leb(buf);
    TRACE(" %u", imm);
    emit1(out_fd, code);
    emit_u32leb4(out_fd, imm);
    return 0;
  }
  case IMM_LABELS: {
    // expand to 4-byte LEBs
    uint32_t count = read_u32leb(buf);
    TRACE(" %u", count);
    emit1(out_fd, code);
    emit_u32leb4(out_fd, count);
    for (uint32_t i = 0; i <= count && buf->ptr < buf->end; i++) {
      uint32_t lbl = read_u32leb(buf);
      emit_u32leb4(out_fd, lbl);
      TRACE(" %u", lbl);
    }
    return 0;
  }
  case IMM_FUNC: // fallthru
  case IMM_LOCAL: // fallthru
  case IMM_GLOBAL: // fallthru
  case IMM_MEMORY: // fallthru
  case IMM_TAG: // fallthru
  case IMM_REFNULLT:
  case IMM_TABLE: {
    uint32_t imm = read_u32leb(buf);
    TRACE(" %u", imm);
    break;
  }
    
  case IMM_NONE: break;
  case IMM_BLOCKT: {
    int32_t imm = read_i32leb(buf);
    if (imm != -64) {
      ERR(" <!illegal blocktype %d>", imm);
      return -5;
    }
    break;
  }
  case IMM_SIG_TABLE: {
    uint32_t sig = read_u32leb(buf);
    TRACE(" %d", sig);
    uint32_t tbl = read_u32leb(buf);
    if (tbl != 0) {
      ERR(" <!illegal table %d>", tbl);\
      return -2;
    }
    break;
  }
  case IMM_MEMARG: {
    byte b = read_u8(buf);
    uint32_t offset = read_u32leb(buf);
    TRACE(" %u", offset);
    break;
  }
  case IMM_I32: {
    int32_t imm = read_i32leb(buf);
    TRACE(" %d", imm);
    break;
  }
  case IMM_F64: {
    if (buf->end - buf->ptr >= 8) {
      uint64_t val = *((uint64_t*)buf->ptr);
      TRACE("  %016" PRIx64, val);
      buf->ptr += 8;
    } else {
      ERR(" <!invalid f64>");
      return -6;
    }
    break;
  }
  case IMM_I64: {
    ssize_t leblen = 0;
    if (buf->ptr >= buf->end) break;
    int64_t imm = decode_i64leb(buf->ptr, buf->end, &leblen);
    if (leblen <= 0) buf->ptr = buf->end; // force failure
    else buf->ptr += leblen;
    TRACE(" %" PRId64 "", imm);
    break;
  }
  case IMM_F32: {
    for (int i = 0; i < 4; i++) {
      byte b = read_u8(buf);
      TRACE(" %02X", b);
    }
    break;
  }
  case IMM_VALTS: {
    int32_t tcode = read_i32leb(buf);
    TRACE(" %s", type_name(tcode));
    break;
  }
  default: break;
  }
  TRACE("\n");
  // emit the bytes as-is
  emit_from(out_fd, start, buf);
  return 0;
}

int transform_body(int out_fd, buffer_t* buf) {
  off_t before = emit_reserved_length(out_fd);
  
  const byte* start = buf->ptr;
  uint32_t cnt = read_u32leb(buf);
  TRACE(" locals %d\n", cnt);
  for (uint32_t i = 0; i < cnt; i++) {
    uint32_t cnt = read_u32leb(buf);
    TRACE("  %u", cnt);
    int32_t type = read_i32leb(buf);
    TRACE(" %s\n", type_name(type));
  }
  emit_from(out_fd, start, buf);
  // disassemble code
  while (buf->ptr < buf->end) {
    int result = transform_bytecode(out_fd, buf);
    if (result != 0) return result;
  }

  emit_patched_length(out_fd, before);
  return 0;
}
