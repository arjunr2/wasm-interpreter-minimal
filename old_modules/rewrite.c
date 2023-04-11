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

typedef struct {
  unsigned is_loop: 1;
  uint32_t start_pc;
  uint32_t num_refs;
  uint32_t capacity;
  int32_t** refs;
} control_entry_t;

void ref_entry(control_entry_t* entry, byte* pcdelta) {
  if (entry->num_refs >= entry->capacity) {
    uint32_t ncapacity = 4 + entry->capacity * 2;
    TRACE("ncapacity = %u\n", ncapacity);
    entry->refs = (int32_t**)realloc(entry->refs, sizeof(int32_t*) * ncapacity);
    entry->capacity = ncapacity;
  }
  entry->refs[entry->num_refs++] = (int32_t*)pcdelta;
}

// Reads what should be a 4-byte LEB indicating a depth.
uint32_t read_label(buffer_t* buf) {
  const byte* before = buf->ptr;
  uint32_t result = read_u32leb(buf);
  uint32_t size = (uint32_t)(buf->ptr - before);
  if (size != 4) ERR("invalid weewasm @+%d: expected 4 byte label, got %u bytes", (int)(before - buf->start), size);
  return result;
}

// Rewrites "br*" to "jmp*"
void rewrite_brs(byte* start, byte* end) {
  buffer_t onstack_buf = { start, start, end };
  buffer_t* buf = &onstack_buf;

  uint32_t control_sp = 0;
  uint32_t capacity = 16;
  control_entry_t* control_stack = calloc(sizeof(control_entry_t), 16);

  {
    // push the initial control entry
    control_entry_t* entry = &control_stack[control_sp++];
    entry->start_pc = (uint32_t)(buf->ptr - buf->start);
    entry->num_refs = 0;
  }

  // process all bytecodes, ignoring all but control flow
  while (buf->ptr < buf->end) {
    int pc = (int)(buf->ptr - buf->start);
    byte b = *buf->ptr;
    switch (b) {
    case WASM_OP_BLOCK: // fall through
    case WASM_OP_LOOP: {
      TRACE("-> +%3d rewrite: control_block, depth=%d\n", pc, control_sp);
      if (control_sp >= capacity) {
        uint32_t ncapacity = capacity * 2;
        control_stack = (control_entry_t*)realloc(control_stack, sizeof(control_entry_t) * ncapacity);
        memset(&control_stack[control_sp], 0, sizeof(control_entry_t) * (ncapacity - control_sp));
      }
      control_entry_t* entry = &control_stack[control_sp++];
      entry->is_loop = b == WASM_OP_LOOP;
      entry->start_pc = (uint32_t)(buf->ptr - buf->start);
      entry->num_refs = 0;
      buf->ptr++;
      read_i32leb(buf); // skip block type
      break;
    }
    case WASM_OP_END: {
      TRACE("-> +%-3d rewrite: end, depth=%d\n", pc, control_sp);
      control_entry_t* entry = &control_stack[control_sp - 1];
      int32_t target_pc = entry->is_loop ? (int32_t)entry->start_pc : (int32_t)(buf->ptr - buf->start);
      for (uint32_t i = 0; i < entry->num_refs; i++) {
        int32_t ref_pc = (int32_t)((const byte*)entry->refs[i] - buf->start);
        TRACE("-> +%-3d rewrite: patch => %d\n", ref_pc, (target_pc - ref_pc));
        *(entry->refs[i]) = target_pc - ref_pc;
      }
      buf->ptr++;
      control_sp--;
      break;
    }
    case WASM_OP_BR: {
      *(byte*)buf->ptr = WASM_OP_JMP;
      buf->ptr++;
      byte* pcdelta = (byte*)buf->ptr;
      uint32_t depth = read_label(buf);
      TRACE("-> +%-3d rewrite: br %u\n", (int)(pcdelta - buf->start), depth);
      if ((buf->ptr - pcdelta) < 4) ERR("!unpadded br");
      control_entry_t* entry = &control_stack[control_sp - depth - 1];
      ref_entry(entry, pcdelta);
      break;
    }
    case WASM_OP_BR_IF: {
      *(byte*)buf->ptr = WASM_OP_JMP_IF;
      buf->ptr++;
      byte* pcdelta = (byte*)buf->ptr;
      uint32_t depth = read_label(buf);
      TRACE("-> +%-3d rewrite: br_if %u\n", (int)(pcdelta - buf->start), depth);
      if ((buf->ptr - pcdelta) < 4) ERR("!unpadded br_if");
      control_entry_t* entry = &control_stack[control_sp - depth - 1];
      ref_entry(entry, pcdelta);
      break;
    }
    case WASM_OP_BR_TABLE: {
      *(byte*)buf->ptr = WASM_OP_JMP_TABLE;
      buf->ptr++;
      uint32_t count = read_u32leb(buf);
      for (uint32_t i = 0; i < count + 1; i++) {
        byte* pcdelta = (byte*)buf->ptr;
        uint32_t depth = read_label(buf);
        TRACE("-> +%-3d rewrite: br_table[%u] %u\n", (int)(pcdelta - buf->start), i, depth);
        if ((buf->ptr - pcdelta) < 4) ERR("!unpadded br_table");
        control_entry_t* entry = &control_stack[control_sp - depth - 1];
        ref_entry(entry, pcdelta);
      }
      break;
    }
    default: {
      TRACE("-> +%-3d rewrite: ...\n", pc);
      skip_bytecode(buf);
      break;
    }
    }
  }

  // free control stack memory
  for (uint32_t i = 0; i < capacity; i++) {
    free(control_stack[i].refs);
  }
  free(control_stack);
}
