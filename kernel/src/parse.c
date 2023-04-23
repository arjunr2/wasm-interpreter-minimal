#include "parse.h"
#include "wasmdefs.h"

#define FN_MAX_SIZE 1048576

#define STATIC_BR_START_BLOCK() \
  static_blocks[static_ctr].op = opcode; \
  static_blocks[static_ctr].start_addr = (byte*) buf->ptr; \
  dyn2static_idxs[scope] = static_ctr;  \
  static_ctr++;

#define STATIC_BR_END_BLOCK() \
  static_blocks[ dyn2static_idxs[scope] ].end_addr = (byte*)buf->ptr - 1;

/*** INSN MACROS ***/

#define SCOPE_BEGIN_INSN() \
  ; \
  /* Skip the blocktype */  \
  uint32_t blocktype = RD_U32();  \
  (void)blocktype;  \
  STATIC_BR_START_BLOCK();  \
  scope++;

#define SCOPE_COM_DEC(s) \
  scope--;  \
  STATIC_BR_END_BLOCK();  \

/***********************/

#define BR_REPLACE_OFFSET() \
    idx = RD_U32();  \
    /* Get static idx of block*/  \
    static_idx = dyn2static_idxs[scope-idx-1]; \
    if (replace_last) { \
      block_list_t block_dets = static_blocks[ static_idx ];  \
      target_addr = ((block_dets.op == WASM_OP_LOOP) ?  \
                            block_dets.start_addr : \
                            block_dets.end_addr); \
      offset = target_addr - buf->ptr;  \
      memcpy((void*)(buf->ptr - 4), &offset, 4); \
    } \
    print_val = replace_last ? offset : idx;  \



static int retval = 0;
/*** Reading intermediate types ***/

static wasm_type_t* read_type_list(uint32_t num, buffer_t *buf) {
  MALLOC(types, wasm_type_t, num);
  /* Add all types for params */
  for (uint32_t j = 0; j < num; j++) {
    types[j] = RD_BYTE();
  }
  return types;
}

/* Read LimitsType */
static wasm_limits_t read_limits(buffer_t *buf) {
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
    retval = RET_ERR;
    return;
  }
  table->limits = read_limits(buf);
}

/* Read GlobalType */
static void read_global_type(wasm_global_decl_t* global, buffer_t *buf) {
  global->type = RD_BYTE();
  global->mutable = RD_BYTE();;
}


/* Function to read i32.const offset since 
  data/elem section only uses these */
static uint32_t decode_flag_and_i32_const_off_expr(buffer_t *buf) {
  // Has to be 0 flag for this
  if (RD_U32() != 0) {  
    ERR("Non-0 flag for data/elem section\n");
    retval = RET_ERR;
    return -1;
  }
  // Has to be i32.const offset for this
  if (RD_BYTE() != WASM_OP_I32_CONST) {
    ERR("Offset for data section has to be \"i32.const\"");
    retval = RET_ERR;
    return -1;
  } 
  // Offset val
  uint32_t offset = RD_U32();
  
  if (RD_BYTE() != WASM_OP_END) {
    ERR("Offset for data section can't find end after i32.const\n");
    retval = RET_ERR;
    return -1;
  }

  return offset;
}

/*** ***/

void decode_type_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  uint32_t num_types = RD_U32();
  MALLOC(sigs, wasm_sig_decl_t, num_types);

  for (uint32_t i = 0; i < num_types; i++) {
    wasm_sig_decl_t *sig = &sigs[i];
    byte kind = RD_BYTE();
    if (kind != WASM_TYPE_FUNC) {
      ERR("Signature must be func-type (0x60)\n");
      retval = RET_ERR;
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
        retval = RET_ERR;
        return;
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
    retval = RET_ERR;
    return;
  }

  module->num_mems = num_mems;
  module->mem_limits = read_limits(buf);
}

void decode_global_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  uint32_t num_globs = RD_U32();
  MALLOC(globals, wasm_global_decl_t, num_globs);

  for (uint32_t i = 0; i < num_globs; i++) {
    wasm_global_decl_t *global = &globals[i];
    read_global_type(global, buf);

    global->init_expr_start = buf->ptr;
    while (RD_BYTE() != WASM_OP_END) { };
    global->init_expr_end = buf->ptr;
  }

  module->num_globals = num_globs;
  module->globals = globals;
}

void decode_export_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  uint32_t num_exps = RD_U32();
  MALLOC(exports, wasm_export_decl_t, num_exps);

  /* String + exp descriptor + idx */
  for (uint32_t i = 0; i < num_exps; i++) {
    wasm_export_decl_t *export = &exports[i];

    /* Export descriptor */
    export->name = RD_NAME(&export->length);
    export->kind = RD_BYTE();
    export->index = RD_U32();
  }

  module->num_exports = num_exps;
  module->exports = exports;
}

void decode_start_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  uint32_t fn_idx = RD_U32();

  module->has_start = true;
  module->start_idx = fn_idx;
}

void decode_element_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  uint32_t num_elem = RD_U32();
  MALLOC(elems, wasm_elems_decl_t, num_elem);

  for (uint32_t i = 0; i < num_elem; i++) {
    wasm_elems_decl_t *elem = &elems[i];
    /* Offset val */
    elem->table_offset = decode_flag_and_i32_const_off_expr(buf);

    /* Element fn idx vector */
    elem->length = RD_U32();
    MALLOC(func_idxs, uint32_t, elem->length);

    for (uint32_t j = 0; j < elem->length; j++) {
      func_idxs[j] = RD_U32();
    }
    elem->func_indexes = func_idxs;
  }

  module->num_elems = num_elem;
  module->elems = elems;
}


static void decode_locals(wasm_func_decl_t *fn, buffer_t *buf) {
  /* Write num local elements */
  uint32_t num_elems = RD_U32();
  MALLOC(locals, wasm_local_decl_t, num_elems);
  
  uint32_t num_locals = 0;
  /* Write local types */
  if (num_elems != 0) {
    for (uint32_t i = 0; i < num_elems; i++) {
      /* Number of locals of type */
      locals[i].count = RD_U32();
      locals[i].type = RD_BYTE();

      num_locals += locals[i].count;
    }
  }
  
  fn->num_local_vec = num_elems;
  fn->num_locals = num_locals;
  fn->local_decl = locals;
}


block_list_t static_blocks[FN_MAX_SIZE];

void decode_expr(buffer_t *buf, bool replace_last) {
  /* For br replacement */
  byte* target_addr;
  uint32_t idx;
  uint32_t static_idx;
  uint32_t offset;
  int32_t print_val;
  /* */

  int static_ctr = 0;
  int num_brs = 0;
  
  MALLOC(dyn2static_idxs, int, FN_MAX_SIZE);

  uint32_t scope = 0;
  // Initial scope
  byte opcode = WASM_OP_BLOCK;
  STATIC_BR_START_BLOCK()
  scope++;

  bool end_of_expr = false;
  while (!end_of_expr) {
    /* Decode insn */
    byte opcode = RD_BYTE();
    opcode_entry_t *entry = &opcode_table[opcode];
    if (entry->invalid) {
      ERR("Invalid opcode: %d (%s)\n", opcode, entry->mnemonic);
      retval = RET_ERR;
      return;
    }
    /* Perform branch replacement/logging */
    switch (opcode) {
      case WASM_OP_BLOCK:		/* "block" BLOCKT */
      case WASM_OP_LOOP:			/* "loop" BLOCKT */
      case WASM_OP_IF:			/* "if" BLOCKT */
          SCOPE_BEGIN_INSN();
          break;

      case WASM_OP_END:			/* "end" */
          SCOPE_COM_DEC(s);
          end_of_expr = (scope == 0);
          break;


      case WASM_OP_BR:			/* "br" LABEL */
      case WASM_OP_BR_IF: 			/* "br_if" LABEL */
        BR_REPLACE_OFFSET();
        num_brs++;
        break;

      case WASM_OP_BR_TABLE: 		/* "br_table" LABELS */
          ;
          /* Label vector */
          uint32_t num_elems = RD_U32();
          for (uint32_t i = 0; i < num_elems; i++) {
            BR_REPLACE_OFFSET();
          }
          /* Label index */
          BR_REPLACE_OFFSET();
          break;
      
      default:
          break;
    }
  }
  FREE(dyn2static_idxs, FN_MAX_SIZE);
}

void decode_code_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  uint32_t num_fn = RD_U32();

  for (uint32_t i = 0; i < num_fn; i++) {
    uint32_t idx = module->num_imports + i;
    /* Fn size */
    uint32_t size = RD_U32();
    const byte* end_insts = buf->ptr + size;

    /* Parse body */
    /* Local section */
    decode_locals(module->funcs + idx, buf);

    module->funcs[idx].code_start = buf->ptr;
    /* Fn body: Instruction decoding */
    decode_expr(buf, false);
    if (buf->ptr != end_insts) {
      ERR("Code parsing misalignment | Ptr -- 0x%lu ; Endinst -- 0x%lu\n",
        buf->ptr - buf->start,
        buf->end - buf->start);
      retval = RET_ERR;
      return;
    }
    module->funcs[idx].code_end = end_insts;
    /* Branch replacement */
    buf->ptr = module->funcs[idx].code_start;
    decode_expr(buf, true);
    if (buf->ptr != end_insts) {
      ERR("Code parsing misalignment | Ptr -- 0x%lu ; Endinst -- 0x%lu\n",
        buf->ptr - buf->start,
        buf->end - buf->start);
      retval = RET_ERR;
      return;
    }
  }
}

void decode_data_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  uint32_t num_datas = RD_U32();
  if (module->has_datacount && (num_datas != module->num_datas)) {
    ERR("Number of data doesn't match datacount: %d, %d\n", num_datas, module->num_datas);
    retval = RET_ERR;
    return;
  }
  MALLOC(datas, wasm_data_decl_t, num_datas);

  for (uint32_t i = 0; i < num_datas; i++) {
    wasm_data_decl_t *data = &datas[i];
    /* Offset val */
    data->mem_offset = decode_flag_and_i32_const_off_expr(buf);
    /* Size val */
    uint32_t sz = RD_U32();

    data->bytes_start = buf->ptr;
    data->bytes_end = buf->ptr + sz;
  }

  module->num_datas = num_datas;
  module->data = datas;
}

void decode_datacount_section(wasm_module_t *module, buffer_t *buf, uint32_t len) {
  module->has_datacount = true;
  module->num_datas = RD_U32();
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
    buffer_t cbuf = {
      .start = buf->ptr, 
      .ptr = buf->ptr, 
      .end = buf->ptr + len
    };

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
  retval = 0;
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


void module_free(wasm_module_t* mod) {

  FREE(mod->table, mod->num_tables);

  for (uint32_t i = 0; i < mod->num_sigs; i++) {
    FREE(mod->sigs[i].params, mod->sigs[i].num_params);
    FREE(mod->sigs[i].results, mod->sigs[i].num_results);
  }
  FREE(mod->sigs, mod->num_sigs);

  for (uint32_t i = 0; i < mod->num_imports; i++) {
    FREE(mod->imports[i].mod_name, mod->imports[i].mod_name_length);
    FREE(mod->imports[i].member_name, mod->imports[i].member_name_length);
  }
  FREE(mod->imports, mod->num_imports);

  for (uint32_t i = 0; i < mod->num_funcs; i++) {
    uint32_t idx = i + mod->num_imports;
    FREE(mod->funcs[idx].local_decl, mod->funcs[idx].num_local_vec);
  }
  FREE(mod->funcs, mod->num_funcs);

  for (uint32_t i = 0; i < mod->num_exports; i++) {
    FREE(mod->exports[i].name, mod->exports[i].length);
  }
  FREE(mod->exports, mod->num_exports);

  FREE(mod->globals, mod->num_globals);

  FREE(mod->data, mod->num_datas);

  for (uint32_t i = 0; i < mod->num_elems; i++) {
    FREE(mod->elems[i].func_indexes, mod->elems[i].length);
  }
  FREE(mod->elems, mod->num_elems);

}
