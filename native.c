#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "native.h"
#include "common.h"
/*
void* native_obj_new(void) {
  TRACE("NATIVE: Object new!\n");
  return malloc(1);
}

void* native_obj_box_i32(uint32_t val) {
  TRACE("NATIVE: Object boxi32!\n");
  return malloc(1);
}

void* native_obj_box_f64(double val) {
  TRACE("NATIVE: Object boxf64!\n");
  return malloc(1);
}

void* native_obj_get(void* obj, void* key) {
  TRACE("NATIVE: Object get!\n");
  return malloc(1);
}

void* native_obj_set(void* obj, void* key, void* val) {
  TRACE("NATIVE: Object set!\n");
  return malloc(1);
}

uint32_t native_i32_unbox(void* obj) {
  TRACE("NATIVE: Object i32unbox!\n");
  return 1;
}

double native_f64_unbox(void* obj) {
  TRACE("NATIVE: Object f64unbox!\n");
  return 1;
}

uint32_t native_obj_eq(void* obj1, void* obj2) {
  TRACE("NATIVE: Object eq!\n");
  return 1;
}
*/

wasm_value_t native_obj_new(void) {
  TRACE("NATIVE: Object new!\n");
  return wasm_ref_value(malloc(1));
}

wasm_value_t native_obj_box_i32(wasm_value_t val) {
  TRACE("NATIVE: Object boxi32!\n");
  return wasm_ref_value(malloc(1));
}

wasm_value_t native_obj_box_f64(wasm_value_t val) {
  TRACE("NATIVE: Object boxf64!\n");
  return wasm_ref_value(malloc(1));
}

wasm_value_t native_obj_get(wasm_value_t obj, wasm_value_t key) {
  TRACE("NATIVE: Object get!\n");
  return wasm_ref_value(malloc(1));
}

wasm_value_t native_obj_set(wasm_value_t obj, wasm_value_t key, wasm_value_t val) {
  TRACE("NATIVE: Object set!\n");
  return wasm_ref_value(malloc(1));
}

wasm_value_t native_i32_unbox(wasm_value_t obj) {
  TRACE("NATIVE: Object i32unbox!\n");
  return wasm_i32_value(1);
}

wasm_value_t native_f64_unbox(wasm_value_t obj) {
  TRACE("NATIVE: Object f64unbox!\n");
  return wasm_f64_value(1);
}

wasm_value_t native_obj_eq(wasm_value_t obj1, wasm_value_t obj2) {
  TRACE("NATIVE: Object eq!\n");
  return wasm_i32_value(1);
}
