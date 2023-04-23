#include <linux/string.h>

#include "common.h"

// The global trace flag
int g_trace = 0;
// The global disassembly flag
int g_disassemble = 0;

#define MORE(b) (((b) & 0x80) != 0)
#define ERROR (len != NULL ? *len = -(ptr - start) : 0), 0

#define BODY(type, mask, legal)						\
  const uint8_t* start = ptr;						\
  type result = 0, shift = 0;						\
  while (ptr < limit) {							\
    uint8_t b = *ptr;							\
    ptr++;								\
    result = result | (((type)b & 0x7F) << shift);			\
    shift += 7;								\
    if (shift > (8*sizeof(type))) {					\
      if (MORE(b) != 0) return ERROR; /* overlong */			\
      uint8_t upper = b & mask;						\
      if (upper != 0 && upper != legal) return ERROR; /* out of range */ \
      if (len != NULL) *len = (ptr - start);				\
      return result;							\
    }									\
    if (MORE(b)) continue;						\
    type rem = ((8*sizeof(type)) - shift);				\
    if (len != NULL) *len = (ptr - start);				\
    return ((0x7F & mask) == legal) ? (result << rem) >> rem : result;	\
  }									\
  return ERROR;


#define STACK_INIT(dst, type) \
  dst = ( type *) malloc(8 * sizeof( type ));

#define PUSH(dst, src)  \
  

int32_t decode_i32leb(const uint8_t* ptr, const uint8_t* limit, ssize_t *len) {
  BODY(int32_t, 0xF8, 0x78);
}

uint32_t decode_u32leb(const uint8_t* ptr, const uint8_t* limit, ssize_t *len) {
  BODY(uint32_t, 0xF8, 0x08);
}

int64_t decode_i64leb(const uint8_t* ptr, const uint8_t* limit, ssize_t *len) {
  BODY(int64_t, 0xFF, 0x7F);
}

uint64_t decode_u64leb(const uint8_t* ptr, const uint8_t* limit, ssize_t *len) {
  BODY(uint64_t, 0xFF, 0x01);
}

uint32_t decode_u32(const uint8_t* ptr, const uint8_t* limit, ssize_t *len) {
  ssize_t remain = limit - ptr;
  if (remain < 4) {
    if (remain > 0) *len = -remain;
    else *len = remain;
    return 0;
  }
  uint32_t b0 = ptr[0];
  uint32_t b1 = ptr[1];
  uint32_t b2 = ptr[2];
  uint32_t b3 = ptr[3];
  *len = 4;
  return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
}



/* Unsigned-32 LEB */
uint32_t read_u32leb(buffer_t* buf) {
  ssize_t leblen = 0;
  if (buf->ptr >= buf->end) return 0;
  uint32_t val = decode_u32leb(buf->ptr, buf->end, &leblen);
  if (leblen <= 0) buf->ptr = buf->end; // force failure
  else buf->ptr += leblen;
  return val;
}

/* Signed-32 LEB */
int32_t read_i32leb(buffer_t* buf) {
  ssize_t leblen = 0;
  if (buf->ptr >= buf->end) return 0;
  int32_t val = decode_i32leb(buf->ptr, buf->end, &leblen);
  if (leblen <= 0) buf->ptr = buf->end; // force failure
  else buf->ptr += leblen;
  return val;
}

/* Unsigned-64 LEB */
uint64_t read_u64leb(buffer_t* buf) {
  ssize_t leblen = 0;
  if (buf->ptr >= buf->end) return 0;
  uint64_t val = decode_u64leb(buf->ptr, buf->end, &leblen);
  if (leblen <= 0) buf->ptr = buf->end; // force failure
  else buf->ptr += leblen;
  return val;
}

/* Signed-64 LEB */
int64_t read_i64leb(buffer_t* buf) {
  ssize_t leblen = 0;
  if (buf->ptr >= buf->end) return 0;
  int64_t val = decode_i64leb(buf->ptr, buf->end, &leblen);
  if (leblen <= 0) buf->ptr = buf->end; // force failure
  else buf->ptr += leblen;
  return val;
}

/* Raw byte */
uint8_t read_u8(buffer_t* buf) {
  if (buf->ptr >= buf->end) {
    ERR("u8 read out of bounds");
    return 0;
  }
  byte val = *buf->ptr;
  buf->ptr++;
  return val;
}

/* Raw-32 */
uint32_t read_u32(buffer_t* buf) {
  ssize_t len;
  uint32_t val = decode_u32(buf->ptr, buf->ptr + 4, &len);
  buf->ptr += 4;
  return val;
}

/* Raw-64 */
uint64_t read_u64(buffer_t* buf) {
  ssize_t len = 0;
  if (buf->ptr + 8 > buf->end) {
    ERR("u64 read out of bounds");
    return 0;
  }
  uint32_t low = decode_u32(buf->ptr, buf->end, &len);
  buf->ptr += len;
  uint32_t high = decode_u32(buf->ptr, buf->end, &len);
  buf->ptr += len;
  uint64_t val = ((uint64_t)(high) << 32) | low;
  return val;
}


/* Name (Unsigned-32leb + string) */
char* read_name(buffer_t* buf, uint32_t* len) {
  uint32_t sz = RD_U32();
  if (buf->ptr + sz > buf->end) {
    ERR("String read out of bounds\n");
    return NULL;
  }

  MALLOC (name, char, sz + 1);
  strncpy (name, buf->ptr, sz);
  name[sz] = 0;
 
  buf->ptr += sz;
  return name;  

}

/* Raw-{num_bytes} */ 
byte* read_bytes(buffer_t* buf, uint32_t num_bytes) {
  if (buf->ptr + num_bytes > buf->end) {
    ERR("bytes read out of bounds\n");
    return NULL;
  }
  MALLOC (bytes, byte, num_bytes);
  memcpy (bytes, buf->ptr, num_bytes);
  return bytes;
}


/*   */
