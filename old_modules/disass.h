#pragma once

#define WASM_TYPE_I32 -1
#define WASM_TYPE_I64 -2
#define WASM_TYPE_F32 -3
#define WASM_TYPE_F64 -4
#define WASM_TYPE_V128 -5
#define WASM_TYPE_EXTERNREF -17
#define WASM_TYPE_FUNCREF -16

#define WASM_IMPORT_FUNC 0
#define WASM_IMPORT_TABLE 1
#define WASM_IMPORT_MEMORY 2
#define WASM_IMPORT_GLOBAL 3

// Returns the string name of a section code.
const char* section_name(byte code);

// Returns the string name of a type code.
const char* type_name(uint32_t code);

// Returns the string name of a type declaration code.
const char* type_decl_name(uint32_t code);

// Returns the name of an import kind.
const char* import_kind_name(uint32_t code);

// Returns the mnemonic name of a bytecode.
const char* bytecode_name(byte b);

// Prints a bytecode to standard out.
void print_bytecode(buffer_t* buf);

// Skips to the next bytecode.
void skip_bytecode(buffer_t* buf);

// Skips over local declarations.
void skip_local_decls(buffer_t* buf);

// Prints data as hexadecimal.
void print_data(buffer_t* buf, uint32_t count);
