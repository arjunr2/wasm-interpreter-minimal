#include <stdio.h>
#include <sys/types.h>

#include "native.h"
#include "common.h"

void* native_obj_new(void) {
  TRACE("NATIVE: Object new!\n");
}

void* native_obj_box_i32(uint32_t val) {
  TRACE("NATIVE: Object boxi32!\n");
}

void* native_obj_box_f64(double val) {
  TRACE("NATIVE: Object boxf64!\n");
}

void* native_obj_get(void* obj, void* key) {
  TRACE("NATIVE: Object get!\n");
}

void* native_obj_set(void* obj, void* key, void* val) {
  TRACE("NATIVE: Object set!\n");
}

uint32_t native_i32_unbox(void* obj) {
  TRACE("NATIVE: Object i32unbox!\n");
}

double native_f64_unbox(void* obj) {
  TRACE("NATIVE: Object f64unbox!\n");
}

uint32_t native_obj_eq(void* obj1, void* obj2) {
  TRACE("NATIVE: Object eq!\n");
}
