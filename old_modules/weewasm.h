#pragma once

#define WASM_MAGIC 0x6d736100u
#define WASM_VERSION 1

#define PAGE_SIZE 65536

// Section constants
#define WASM_SECT_TYPE 1
#define WASM_SECT_IMPORT 2
#define WASM_SECT_FUNCTION 3
#define WASM_SECT_TABLE 4
#define WASM_SECT_MEMORY 5
#define WASM_SECT_GLOBAL 6
#define WASM_SECT_EXPORT 7
#define WASM_SECT_START 8
#define WASM_SECT_ELEMENT 9
#define WASM_SECT_CODE 10
#define WASM_SECT_DATA 11

// Opcode constants
#define WASM_OP_UNREACHABLE		0x00 /* "unreachable" */
#define WASM_OP_NOP			0x01 /* "nop" */
#define WASM_OP_BLOCK			0x02 /* "block" BLOCKT */
#define WASM_OP_LOOP			0x03 /* "loop" BLOCKT */
#define WASM_OP_IF			0x04 /* "if" BLOCKT */
#define WASM_OP_ELSE			0x05 /* "else" */
#define WASM_OP_END			0x0B /* "end" */
#define WASM_OP_BR			0x0C /* "br" LABEL */
#define WASM_OP_BR_IF			0x0D /* "br_if" LABEL */
#define WASM_OP_BR_TABLE		0x0E /* "br_table" LABELS */
#define WASM_OP_RETURN			0x0F /* "return" */
#define WASM_OP_CALL			0x10 /* "call" FUNC */
#define WASM_OP_CALL_INDIRECT		0x11 /* "call_indirect" SIG_TABLE */
#define WASM_OP_DROP			0x1A /* "drop" */
#define WASM_OP_SELECT			0x1B /* "select" */
#define WASM_OP_LOCAL_GET		0x20 /* "local.get" LOCAL */
#define WASM_OP_LOCAL_SET		0x21 /* "local.set" LOCAL */
#define WASM_OP_LOCAL_TEE		0x22 /* "local.tee" LOCAL */
#define WASM_OP_GLOBAL_GET		0x23 /* "global.get" GLOBAL */
#define WASM_OP_GLOBAL_SET		0x24 /* "global.set" GLOBAL */
#define WASM_OP_TABLE_GET		0x25 /* "table.get" TABLE */
#define WASM_OP_TABLE_SET		0x26 /* "table.set" TABLE */
#define WASM_OP_I32_LOAD		0x28 /* "i32.load" MEMARG */
#define WASM_OP_F64_LOAD		0x2B /* "f64.load" MEMARG */
#define WASM_OP_I32_LOAD8_S		0x2C /* "i32.load8_s" MEMARG */
#define WASM_OP_I32_LOAD8_U		0x2D /* "i32.load8_u" MEMARG */
#define WASM_OP_I32_LOAD16_S		0x2E /* "i32.load16_s" MEMARG */
#define WASM_OP_I32_LOAD16_U		0x2F /* "i32.load16_u" MEMARG */
#define WASM_OP_I32_STORE		0x36 /* "i32.store" MEMARG */
#define WASM_OP_F64_STORE		0x39 /* "f64.store" MEMARG */
#define WASM_OP_I32_STORE8		0x3A /* "i32.store8" MEMARG */
#define WASM_OP_I32_STORE16		0x3B /* "i32.store16" MEMARG */
#define WASM_OP_I32_CONST		0x41 /* "i32.const" I32 */
#define WASM_OP_F64_CONST		0x44 /* "f64.const" F64 */
#define WASM_OP_I32_EQZ			0x45 /* "i32.eqz" */
#define WASM_OP_I32_EQ			0x46 /* "i32.eq" */
#define WASM_OP_I32_NE			0x47 /* "i32.ne" */
#define WASM_OP_I32_LT_S		0x48 /* "i32.lt_s" */
#define WASM_OP_I32_LT_U		0x49 /* "i32.lt_u" */
#define WASM_OP_I32_GT_S		0x4A /* "i32.gt_s" */
#define WASM_OP_I32_GT_U		0x4B /* "i32.gt_u" */
#define WASM_OP_I32_LE_S		0x4C /* "i32.le_s" */
#define WASM_OP_I32_LE_U		0x4D /* "i32.le_u" */
#define WASM_OP_I32_GE_S		0x4E /* "i32.ge_s" */
#define WASM_OP_I32_GE_U		0x4F /* "i32.ge_u" */
#define WASM_OP_F64_EQ			0x61 /* "f64.eq" */
#define WASM_OP_F64_NE			0x62 /* "f64.ne" */
#define WASM_OP_F64_LT			0x63 /* "f64.lt" */
#define WASM_OP_F64_GT			0x64 /* "f64.gt" */
#define WASM_OP_F64_LE			0x65 /* "f64.le" */
#define WASM_OP_F64_GE			0x66 /* "f64.ge" */
#define WASM_OP_I32_CLZ			0x67 /* "i32.clz" */
#define WASM_OP_I32_CTZ			0x68 /* "i32.ctz" */
#define WASM_OP_I32_POPCNT		0x69 /* "i32.popcnt" */
#define WASM_OP_I32_ADD			0x6A /* "i32.add" */
#define WASM_OP_I32_SUB			0x6B /* "i32.sub" */
#define WASM_OP_I32_MUL			0x6C /* "i32.mul" */
#define WASM_OP_I32_DIV_S		0x6D /* "i32.div_s" */
#define WASM_OP_I32_DIV_U		0x6E /* "i32.div_u" */
#define WASM_OP_I32_REM_S		0x6F /* "i32.rem_s" */
#define WASM_OP_I32_REM_U		0x70 /* "i32.rem_u" */
#define WASM_OP_I32_AND			0x71 /* "i32.and" */
#define WASM_OP_I32_OR			0x72 /* "i32.or" */
#define WASM_OP_I32_XOR			0x73 /* "i32.xor" */
#define WASM_OP_I32_SHL			0x74 /* "i32.shl" */
#define WASM_OP_I32_SHR_S		0x75 /* "i32.shr_s" */
#define WASM_OP_I32_SHR_U		0x76 /* "i32.shr_u" */
#define WASM_OP_I32_ROTL		0x77 /* "i32.rotl" */
#define WASM_OP_I32_ROTR		0x78 /* "i32.rotr" */
#define WASM_OP_F64_ADD			0xA0 /* "f64.add" */
#define WASM_OP_F64_SUB			0xA1 /* "f64.sub" */
#define WASM_OP_F64_MUL			0xA2 /* "f64.mul" */
#define WASM_OP_F64_DIV			0xA3 /* "f64.div" */
#define WASM_OP_I32_TRUNC_F64_S		0xAA /* "i32.trunc_f64_s" */
#define WASM_OP_I32_TRUNC_F64_U		0xAB /* "i32.trunc_f64_u" */
#define WASM_OP_F64_CONVERT_I32_S	0xB7 /* "f64.convert_i32_s" */
#define WASM_OP_F64_CONVERT_I32_U	0xB8 /* "f64.convert_i32_u" */
#define WASM_OP_F64_CONVERT_I64_S	0xB9 /* "f64.convert_i64_s" */
#define WASM_OP_F64_CONVERT_I64_U	0xBA /* "f64.convert_i64_u" */
#define WASM_OP_I32_EXTEND8_S		0xC0 /* "i32.extend8_s" */
#define WASM_OP_I32_EXTEND16_S		0xC1 /* "i32.extend16_s" */


// Value decoding
#define WASM_TYPE_I32 0x7F
#define WASM_TYPE_F64 0x7C
#define WASM_TYPE_EXTERNREF 0x6F
#define WASM_TYPE_FUNCREF 0x70

#define WASM_TYPE_GLOBAL_MUTABLE 0x01

// Kind decoding
#define WASM_KIND_FUNC 0x60

// Import/export desc decoding
#define WASM_IE_DESC_FUNC 0x00
#define WASM_IE_DESC_TABLE 0x01
#define WASM_IE_DESC_MEM 0x02
#define WASM_IE_DESC_GLOBAL 0x03

#define TARGET_LABEL_DEF(label) [label] = &&TARGET_##label

#define CREATE_TARGET_JUMP_TABLE() \
  static void *target_jump_table[] = {  \
    TARGET_LABEL_DEF(WASM_OP_UNREACHABLE), \
    TARGET_LABEL_DEF(WASM_OP_NOP), \
    TARGET_LABEL_DEF(WASM_OP_BLOCK), \
    TARGET_LABEL_DEF(WASM_OP_LOOP), \
    TARGET_LABEL_DEF(WASM_OP_IF), \
    TARGET_LABEL_DEF(WASM_OP_ELSE), \
    TARGET_LABEL_DEF(WASM_OP_END), \
    TARGET_LABEL_DEF(WASM_OP_BR), \
    TARGET_LABEL_DEF(WASM_OP_BR_IF), \
    TARGET_LABEL_DEF(WASM_OP_BR_TABLE), \
    TARGET_LABEL_DEF(WASM_OP_RETURN), \
    TARGET_LABEL_DEF(WASM_OP_CALL), \
    TARGET_LABEL_DEF(WASM_OP_CALL_INDIRECT), \
    TARGET_LABEL_DEF(WASM_OP_DROP), \
    TARGET_LABEL_DEF(WASM_OP_SELECT), \
    TARGET_LABEL_DEF(WASM_OP_LOCAL_GET), \
    TARGET_LABEL_DEF(WASM_OP_LOCAL_SET), \
    TARGET_LABEL_DEF(WASM_OP_LOCAL_TEE), \
    TARGET_LABEL_DEF(WASM_OP_GLOBAL_GET), \
    TARGET_LABEL_DEF(WASM_OP_GLOBAL_SET), \
    TARGET_LABEL_DEF(WASM_OP_TABLE_GET), \
    TARGET_LABEL_DEF(WASM_OP_TABLE_SET), \
    TARGET_LABEL_DEF(WASM_OP_I32_LOAD), \
    TARGET_LABEL_DEF(WASM_OP_F64_LOAD), \
    TARGET_LABEL_DEF(WASM_OP_I32_LOAD8_S), \
    TARGET_LABEL_DEF(WASM_OP_I32_LOAD8_U), \
    TARGET_LABEL_DEF(WASM_OP_I32_LOAD16_S), \
    TARGET_LABEL_DEF(WASM_OP_I32_LOAD16_U), \
    TARGET_LABEL_DEF(WASM_OP_I32_STORE), \
    TARGET_LABEL_DEF(WASM_OP_F64_STORE), \
    TARGET_LABEL_DEF(WASM_OP_I32_STORE8), \
    TARGET_LABEL_DEF(WASM_OP_I32_STORE16), \
    TARGET_LABEL_DEF(WASM_OP_I32_CONST), \
    TARGET_LABEL_DEF(WASM_OP_F64_CONST), \
    TARGET_LABEL_DEF(WASM_OP_I32_EQZ), \
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
    TARGET_LABEL_DEF(WASM_OP_F64_EQ), \
    TARGET_LABEL_DEF(WASM_OP_F64_NE), \
    TARGET_LABEL_DEF(WASM_OP_F64_LT), \
    TARGET_LABEL_DEF(WASM_OP_F64_GT), \
    TARGET_LABEL_DEF(WASM_OP_F64_LE), \
    TARGET_LABEL_DEF(WASM_OP_F64_GE), \
    TARGET_LABEL_DEF(WASM_OP_I32_CLZ), \
    TARGET_LABEL_DEF(WASM_OP_I32_CTZ), \
    TARGET_LABEL_DEF(WASM_OP_I32_POPCNT), \
    TARGET_LABEL_DEF(WASM_OP_I32_ADD), \
    TARGET_LABEL_DEF(WASM_OP_I32_SUB), \
    TARGET_LABEL_DEF(WASM_OP_I32_MUL), \
    TARGET_LABEL_DEF(WASM_OP_I32_DIV_S), \
    TARGET_LABEL_DEF(WASM_OP_I32_DIV_U), \
    TARGET_LABEL_DEF(WASM_OP_I32_REM_S), \
    TARGET_LABEL_DEF(WASM_OP_I32_REM_U), \
    TARGET_LABEL_DEF(WASM_OP_I32_AND), \
    TARGET_LABEL_DEF(WASM_OP_I32_OR), \
    TARGET_LABEL_DEF(WASM_OP_I32_XOR), \
    TARGET_LABEL_DEF(WASM_OP_I32_SHL), \
    TARGET_LABEL_DEF(WASM_OP_I32_SHR_S), \
    TARGET_LABEL_DEF(WASM_OP_I32_SHR_U), \
    TARGET_LABEL_DEF(WASM_OP_I32_ROTL), \
    TARGET_LABEL_DEF(WASM_OP_I32_ROTR), \
    TARGET_LABEL_DEF(WASM_OP_F64_ADD), \
    TARGET_LABEL_DEF(WASM_OP_F64_SUB), \
    TARGET_LABEL_DEF(WASM_OP_F64_MUL), \
    TARGET_LABEL_DEF(WASM_OP_F64_DIV), \
    TARGET_LABEL_DEF(WASM_OP_I32_TRUNC_F64_S), \
    TARGET_LABEL_DEF(WASM_OP_I32_TRUNC_F64_U), \
    TARGET_LABEL_DEF(WASM_OP_F64_CONVERT_I32_S), \
    TARGET_LABEL_DEF(WASM_OP_F64_CONVERT_I32_U), \
    TARGET_LABEL_DEF(WASM_OP_F64_CONVERT_I64_S), \
    TARGET_LABEL_DEF(WASM_OP_F64_CONVERT_I64_U), \
    TARGET_LABEL_DEF(WASM_OP_I32_EXTEND8_S), \
    TARGET_LABEL_DEF(WASM_OP_I32_EXTEND16_S), \
  }

// Constant array
static const char* opcode_names[] = {
    [WASM_OP_UNREACHABLE]       = "unreachable",
    [WASM_OP_NOP]               = "nop",			
    [WASM_OP_BLOCK]             = "block",
    [WASM_OP_LOOP]              = "loop",			
    [WASM_OP_IF]                = "if",			
    [WASM_OP_ELSE]              = "else",			
    [WASM_OP_END]               = "end",
    [WASM_OP_BR]                = "br",
    [WASM_OP_BR_IF]             = "br_if",			
    [WASM_OP_BR_TABLE]          = "br_table",
    [WASM_OP_RETURN]            = "return",
    [WASM_OP_CALL]              = "call",
    [WASM_OP_CALL_INDIRECT]     = "call_indirect",	
    [WASM_OP_DROP]			        = "drop",
    [WASM_OP_SELECT]			      = "select",
    [WASM_OP_LOCAL_GET]		      = "local.get",
    [WASM_OP_LOCAL_SET]		      = "local.set",
    [WASM_OP_LOCAL_TEE]		      = "local.tee",
    [WASM_OP_GLOBAL_GET]		    = "global.get",
    [WASM_OP_GLOBAL_SET]		    = "global.set",
    [WASM_OP_TABLE_GET]		      = "table.get",
    [WASM_OP_TABLE_SET]		      = "table.set",
    [WASM_OP_I32_LOAD]		      = "i32.load",
    [WASM_OP_F64_LOAD]		      = "f64.load",
    [WASM_OP_I32_LOAD8_S]		    = "i32.load8_s",
    [WASM_OP_I32_LOAD8_U]		    = "i32.load8_u",
    [WASM_OP_I32_LOAD16_S]		  = "i32.load16_s",
    [WASM_OP_I32_LOAD16_U]		  = "i32.load16_u",
    [WASM_OP_I32_STORE]		      = "i32.store",
    [WASM_OP_F64_STORE]		      = "f64.store",
    [WASM_OP_I32_STORE8]		    = "i32.store8",
    [WASM_OP_I32_STORE16]		    = "i32.store16",
    [WASM_OP_I32_CONST]		      = "i32.const",
    [WASM_OP_F64_CONST]		      = "f64.const",
    [WASM_OP_I32_EQZ]			      = "i32.eqz",
    [WASM_OP_I32_EQ]		        = "i32.eq",
    [WASM_OP_I32_NE]		        = "i32.ne",
    [WASM_OP_I32_LT_S]	        = "i32.lt_s",
    [WASM_OP_I32_LT_U]	        = "i32.lt_u",
    [WASM_OP_I32_GT_S]	        = "i32.gt_s",
    [WASM_OP_I32_GT_U]	        = "i32.gt_u",
    [WASM_OP_I32_LE_S]	        = "i32.le_s",
    [WASM_OP_I32_LE_U]	        = "i32.le_u",
    [WASM_OP_I32_GE_S]	        = "i32.ge_s",
    [WASM_OP_I32_GE_U]	        = "i32.ge_u",
    [WASM_OP_F64_EQ]			      = "f64.eq",
    [WASM_OP_F64_NE]			      = "f64.ne",
    [WASM_OP_F64_LT]			      = "f64.lt",
    [WASM_OP_F64_GT]			      = "f64.gt",
    [WASM_OP_F64_LE]			      = "f64.le",
    [WASM_OP_F64_GE]		        = "f64.ge",
    [WASM_OP_I32_CLZ]	          = "i32.clz",
    [WASM_OP_I32_CTZ]	          = "i32.ctz",
    [WASM_OP_I32_POPCNT]	      = "i32.popcnt",
    [WASM_OP_I32_ADD]		        = "i32.add",
    [WASM_OP_I32_SUB]		        = "i32.sub",
    [WASM_OP_I32_MUL]		        = "i32.mul",
    [WASM_OP_I32_DIV_S]	        = "i32.div_s",
    [WASM_OP_I32_DIV_U]	        = "i32.div_u",
    [WASM_OP_I32_REM_S]	        = "i32.rem_s",
    [WASM_OP_I32_REM_U]	        = "i32.rem_u",
    [WASM_OP_I32_AND]		        = "i32.and",
    [WASM_OP_I32_OR]			      = "i32.or",
    [WASM_OP_I32_XOR]		        = "i32.xor",
    [WASM_OP_I32_SHL]		        = "i32.shl",
    [WASM_OP_I32_SHR_S]         = "i32.shr_s",
    [WASM_OP_I32_SHR_U]         = "i32.shr_u",
    [WASM_OP_I32_ROTL]	        = "i32.rotl",
    [WASM_OP_I32_ROTR]	        = "i32.rotr",
    [WASM_OP_F64_ADD]		        = "f64.add",
    [WASM_OP_F64_SUB]		        = "f64.sub",
    [WASM_OP_F64_MUL]		        = "f64.mul",
    [WASM_OP_F64_DIV]		        = "f64.div",
    [WASM_OP_I32_TRUNC_F64_S]   = "i32.trunc_f64_s",
    [WASM_OP_I32_TRUNC_F64_U]   = "i32.trunc_f64_u",
    [WASM_OP_F64_CONVERT_I32_S] = "f64.convert_i32_s",
    [WASM_OP_F64_CONVERT_I32_U] = "f64.convert_i32_u",
    [WASM_OP_F64_CONVERT_I64_S] = "f64.convert_i64_s",
    [WASM_OP_F64_CONVERT_I64_U] = "f64.convert_i64_u",
    [WASM_OP_I32_EXTEND8_S]     = "i32.extend8_s",
    [WASM_OP_I32_EXTEND16_S]    = "i32.extend16_s"
};
