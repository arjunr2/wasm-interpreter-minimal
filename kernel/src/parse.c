#include "parse.h"
#include "wasmdefs.h"


void decode_type_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}

void decode_import_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}

void decode_function_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}

void decode_table_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}

void decode_memory_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}

void decode_global_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}

void decode_export_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}

void decode_start_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}

void decode_element_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}

void decode_code_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}

void decode_data_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}

void decode_datacount_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}

void decode_custom_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  buf->ptr += len;
}



/* Decode all sections in WebAssembly binary */
int decode_sections(wasm_module_t* module, buffer_t *buf) {

	uint32_t magic = RD_U32_RAW();
	if (magic != WASM_MAGIC) {
		ERR("MAGIC incorrect: %u\n", magic);
		return RET_ERR;
	}

	uint32_t version = RD_U32_RAW();
	if (version != WASM_VERSION) {
		ERR("VERSION value incorrect: %u\n", version);
		return RET_ERR;
	}

	while (buf->ptr < buf->end) {
		wasm_section_t section_id = (wasm_section_t) RD_BYTE();
		uint32_t len = RD_U32();

		TRACE("Found section \"%s\", len: %d\n", wasm_section_name(section_id), len);
    buffer_t cbuf = {buf->ptr, buf->ptr, buf->ptr + len};

    #define DECODE_CALL(sec)  decode_##sec##_section (module, &cbuf, len); break;
    switch (section_id) {
      case WASM_SECT_TYPE:      DECODE_CALL(type);
      case WASM_SECT_IMPORT:    DECODE_CALL(import);
      case WASM_SECT_FUNCTION:  DECODE_CALL(function);
      case WASM_SECT_TABLE:     DECODE_CALL(table);
      case WASM_SECT_MEMORY:    DECODE_CALL(memory);
      case WASM_SECT_GLOBAL:    DECODE_CALL(global);
      case WASM_SECT_EXPORT:    DECODE_CALL(export);
      case WASM_SECT_START:     DECODE_CALL(start);
      case WASM_SECT_ELEMENT:   DECODE_CALL(element);
      case WASM_SECT_CODE:      DECODE_CALL(code);
      case WASM_SECT_DATA:      DECODE_CALL(data);
      case WASM_SECT_DATACOUNT: DECODE_CALL(datacount);
      case WASM_SECT_CUSTOM:    DECODE_CALL(custom);
      default:
        ERR("Unknown section id: %u\n", section_id);
				return RET_ERR;
    }

    if (cbuf.ptr != cbuf.end) {
      ERR("Section \"%s\" not aligned after parsing -- start:%lu, ptr:%lu, end:%lu\n",
          wasm_section_name(section_id),
          cbuf.start - buf->start,
          cbuf.ptr - buf->start,
          cbuf.end - buf->start);
			return RET_ERR;
    }
    // Advance section
    buf->ptr = cbuf.ptr;
	}

  return RET_SUCCESS;
}


int parse(wasm_module_t *module, buffer_t buf) {
  buffer_t fbuf = buf;
	if (decode_sections(module, &fbuf) == -1) {
    return RET_ERR;
  }
	if (fbuf.ptr != fbuf.end) {
		ERR("Unexpected end | Cur -- 0x%lu ; End -- 0x%lu\n", 
				fbuf.ptr - fbuf.start,
				fbuf.end - fbuf.start);
		return RET_ERR;
	}
	return 0;
}
