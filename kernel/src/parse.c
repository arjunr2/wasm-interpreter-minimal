#include "parse.h"
#include "wasmdefs.h"


static wasm_type_t* read_type_list(uint32_t num, buffer_t *buf) {
  MALLOC(types, wasm_type_t, num);
  /* Add all types for params */
  for (uint32_t j = 0; j < num; j++) {
    types[j] = RD_BYTE();
  }
  return types;
}

/* Read LimitsType */
wasm_limits_t read_limits(buffer_t *buf) {
  byte has_max = RD_BYTE();
  /* Min */
  uint32_t initial = RD_U32();
  uint32_t max = has_max ? RD_U32() : -1;

  wasm_limits_t limits = { .initial = initial, .max = max, .has_max = has_max };
  return limits;
}

/* Read TableType */
static void read_table_type(wasm_table_decl_t *table, buffer_t *buf) {
  byte reftype = RD_BYTE();
  if ((reftype != WASM_TYPE_FUNCREF) && (reftype != WASM_TYPE_EXTERNREF)) {
    ERR("Invalid table reftype: %d\n", reftype);
    return;
  }
  table->limits = read_limits(buf);
}


void decode_type_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  uint32_t num_types = RD_U32();
  MALLOC(sigs, wasm_sig_decl_t, num_types);

  for (uint32_t i = 0; i < num_types; i++) {
    wasm_sig_decl_t *sig = &sigs[i];
    byte kind = RD_BYTE();
    if (kind != WASM_TYPE_FUNC) {
      ERR("Signature must be func-type (0x60)\n");
      return;
    }
    sig->num_params = RD_U32();
    sig->params = read_type_list(sig->num_params, buf);
    sig->num_results = RD_U32();
    sig->results = read_type_list(sig->num_results, buf);
  }

  module->num_sigs = num_types;
  module->sigs = sigs;
}

void decode_import_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  uint32_t num_imports = RD_U32();

  MALLOC(imports, wasm_import_decl_t, num_imports);
  for (uint32_t i = 0; i < num_imports; i++) {
    wasm_import_decl_t *import = &imports[i];

    /* Module */
    /* Import names */
    import->mod_name = RD_NAME(&import->mod_name_length);
    import->member_name = RD_NAME(&import->member_name_length);

    /* Import type and args */    
    import->kind = RD_BYTE();
    switch(import->kind) {
      case KIND_FUNC: import->index = RD_U32(); break;
      default:
        ERR("Invalid kind for import: %d\n", import->kind);
    }
  }

  module->num_imports = num_imports;
  module->imports = imports;
}


void decode_function_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  uint32_t num_funcs = RD_U32();
  uint32_t num_imports = module->num_imports;

  MALLOC(funcs, wasm_func_decl_t, num_imports + num_funcs);

  for (uint32_t i = 0; i < num_imports; i++) {
    wasm_func_decl_t *func = &funcs[i];
    func->sig_index = module->imports[i].index;
    func->sig = &module->sigs[func->sig_index];
  }
  for (uint32_t i = 0; i < num_funcs; i++) {
    wasm_func_decl_t *func = &funcs[i + num_imports];

    /* Get signature idx */
    func->sig_index = RD_U32();
    func->sig = &module->sigs[func->sig_index];
  }

  module->num_funcs = num_funcs;
  module->funcs = funcs;
}

void decode_table_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  uint32_t num_tabs = RD_U32();

  MALLOC(tables, wasm_table_decl_t, num_tabs);

  for (uint32_t i = 0; i < num_tabs; i++) {
    read_table_type(&tables[i], buf);
  }

  module->num_tables = num_tabs;
  module->table = tables;
}

void decode_memory_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  uint32_t num_mems = read_u32leb(buf);
  if (num_mems != 1) {
    ERR("Memory component has to be 1!\n");
    return;
  }

  module->num_mems = num_mems;
  module->mem_limits = read_limits(buf);
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
