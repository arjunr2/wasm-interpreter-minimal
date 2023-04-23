#pragma once

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t byte;
typedef struct {
  const byte* start;
  const byte* ptr;
  const byte* end;
} buffer_t;


// The global trace flag.
extern int g_trace;
extern int g_disassemble;

// Helper macros to trace and to print errors.
#define TRACE(...) printf(__VA_ARGS__);
#define PRINT(...) printf(__VA_ARGS__);
#define ERR(...) fprintf(stderr, __VA_ARGS__)

#define MALLOC(dest, dt, num) \
  dt* dest;  \
  if (num != 0) { \
    dest = ( dt* ) malloc ((num) * sizeof( dt )); \
  }

#define FREE(val, num)  \
  if (num != 0) { \
    free((void*)val);  \
  }


#define FP_U64_BITS(val) ({	\
		(union { double d; uint64_t u; }) {val}.u;	\
		})

/*** Parsing macros ***/
#define RD_U32()        read_u32leb(buf)
#define RD_I32()        read_i32leb(buf)
#define RD_U64()        read_u64leb(buf)
#define RD_I64()        read_i64leb(buf)
#define RD_NAME(len_ptr)  read_name(buf, len_ptr)
#define RD_BYTESTR(len) read_bytes(buf, len)

#define RD_BYTE()       read_u8(buf)
#define RD_U32_RAW()    read_u32(buf)
#define RD_U64_RAW()    read_u64(buf)

#define VALIDATE_OP(b) {  \
  opcode_entry_t op_entry = opcode_table[b]; \
  if (op_entry.invalid || (op_entry.mnemonic == 0)) { \
    ERR("Unimplemented opcode %d: %s\n", b, op_entry.mnemonic);  \
    throw std::runtime_error("Unimplemented");  \
  } \
}

#define RD_OPCODE() ({  \
  uint16_t lb = RD_BYTE();  \
  VALIDATE_OP(lb); \
  if ((lb >= 0xFB) && (lb <= 0xFE)) {  \
    lb = ((lb << 8) + RD_BYTE()); \
    VALIDATE_OP(lb); \
  } \
  lb; \
})
/********************/


// Limit file size to something reasonable.
#define MAX_FILE_SIZE 2000000000

// Decode LEB128 values, enforcing the specified signedness and maximum range.
// The argument {ptr} will be advanced to point after the decoded value.
// If an error occurs (e.g. unterminated LEB, invalid value), {ptr} will be set to
// NULL.
int32_t  decode_i32leb(const byte* ptr, const byte* limit, ssize_t* len);
uint32_t decode_u32leb(const byte* ptr, const byte* limit, ssize_t* len);
int64_t  decode_i64leb(const byte* ptr, const byte* limit, ssize_t* len);
uint64_t decode_u64leb(const byte* ptr, const byte* limit, ssize_t* len);

// Decode fixed-size integer values.
uint32_t decode_u32(const byte* ptr, const byte* limit, ssize_t* len);

// Load a file into memory, initializing pointers to the start and end of
// the memory range containing the data. Returns < 0 on failure.
ssize_t load_file(const char* path, byte** start, byte** end);

// Unload a file previously loaded into memory using {load_file}.
ssize_t unload_file(byte** start, byte** end);


/* Read an unsigned(u)/signed(i) X-bit LEB, advancing the {ptr} in buffer */
uint32_t read_u32leb(buffer_t* buf);
int32_t read_i32leb(buffer_t* buf);
uint64_t read_u64leb(buffer_t* buf);
int64_t read_i64leb(buffer_t* buf);

/* Read a raw X-bit value, advancing the {ptr} in buffer*/
uint8_t read_u8(buffer_t* buf);
uint32_t read_u32(buffer_t* buf);
uint64_t read_u64(buffer_t* buf);

/* Read a name (32leb + string), advancing the {ptr} in buffer */
char* read_name(buffer_t* buf, uint32_t *len);
/* Read num_bytes, advancing the {ptr} in buffer */
byte* read_bytes(buffer_t* buf, uint32_t num_bytes);

