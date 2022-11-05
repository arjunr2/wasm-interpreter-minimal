#include "ir.h"

//void* native_obj_new(void);
//void* native_obj_box_i32(uint32_t val);
//void* native_obj_box_f64(double val);
//void* native_obj_get(void* obj, void* key);
//void* native_obj_set(void* obj, void* key, void* val);
//uint32_t native_i32_unbox(void* obj);
//double native_f64_unbox(void* obj);
//uint32_t native_obj_eq(void* obj1, void* obj2);

wasm_value_t native_obj_new(void);
wasm_value_t native_obj_box_i32(wasm_value_t value);
wasm_value_t native_obj_box_f64(wasm_value_t value);
wasm_value_t native_obj_get(wasm_value_t obj, wasm_value_t key);
wasm_value_t native_obj_set(wasm_value_t obj, wasm_value_t key, wasm_value_t val);
wasm_value_t native_i32_unbox(wasm_value_t obj);
wasm_value_t native_f64_unbox(wasm_value_t obj);
wasm_value_t native_obj_eq(wasm_value_t obj1, wasm_value_t obj2);

