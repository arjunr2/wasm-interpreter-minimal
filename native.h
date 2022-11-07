#include "ir.h"

wasm_value_t native_obj_new(void);
wasm_value_t native_obj_box_i32(wasm_value_t value);
wasm_value_t native_obj_box_f64(wasm_value_t value);
wasm_value_t native_obj_get(wasm_value_t obj_ref, wasm_value_t key_ref);
wasm_value_t native_obj_set(wasm_value_t obj_ref, wasm_value_t key_ref, wasm_value_t val_ref);
wasm_value_t native_i32_unbox(wasm_value_t obj_ref);
wasm_value_t native_f64_unbox(wasm_value_t obj_ref);
wasm_value_t native_obj_eq(wasm_value_t obj1_ref, wasm_value_t obj2_ref);

