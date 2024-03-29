#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "native.h"
#include "common.h"
#include "object.h"


wasm_value_t native_obj_new(void) {
  TRACE("NATIVE: Object new!\n");

  Object *obj = create_object(OBJECT);
  ht_init(&obj->val.ht);
  return wasm_ref_value(obj);
}

wasm_value_t native_obj_box_i32(wasm_value_t value) {
  TRACE("NATIVE: Object boxi32!\n");
  
  Object *obj = create_object(BOX_I32);
  obj->val.i32 = value.val.i32;
  return wasm_ref_value(obj);
}

wasm_value_t native_obj_box_f64(wasm_value_t value) {
  TRACE("NATIVE: Object boxf64!\n");

  Object *obj = create_object(BOX_F64);
  obj->val.f64 = value.val.f64;
  return wasm_ref_value(obj);
}

wasm_value_t native_obj_get(wasm_value_t obj_ref, wasm_value_t key_ref) {
  TRACE("NATIVE: Object get!\n");

  Object *obj = (Object*) obj_ref.val.ref;
  Object *key = (Object*) key_ref.val.ref;
  if ((obj == NULL) || (key == NULL) || is_boxed(obj)) {
    TRAP();
  }

  Object *value = ht_find(&obj->val.ht, key);
  return wasm_ref_value(value);
}

// TODO
wasm_value_t native_obj_set(wasm_value_t obj_ref, wasm_value_t key_ref, wasm_value_t val_ref) {
  TRACE("NATIVE: Object set!\n");

  Object *obj = (Object*) obj_ref.val.ref;
  Object *key = (Object*) key_ref.val.ref;
  Object *val = (Object*) val_ref.val.ref;
  if ((obj == NULL) || (key == NULL) || is_boxed(obj)) {
    TRAP();
  }

  ht_insert(&obj->val.ht, key, val);
  return wasm_ref_value(NULL);
}


wasm_value_t native_i32_unbox(wasm_value_t obj_ref) {
  TRACE("NATIVE: Object i32unbox!\n");

  Object *obj = (Object*) obj_ref.val.ref;
  if ((obj == NULL) || !is_type(obj, BOX_I32)) {
    TRAP();
  }

  return wasm_i32_value(obj->val.i32);
}

wasm_value_t native_f64_unbox(wasm_value_t obj_ref) {
  TRACE("NATIVE: Object f64unbox!\n");

  Object *obj = (Object*) obj_ref.val.ref;
  if ((obj == NULL) || !is_type(obj, BOX_F64)) {
    TRAP();
  }

  return wasm_f64_value(obj->val.f64);
}

wasm_value_t native_obj_eq(wasm_value_t obj1_ref, wasm_value_t obj2_ref) {
  TRACE("NATIVE: Object eq!\n");

  Object *obj1 = (Object*) obj1_ref.val.ref;
  Object *obj2 = (Object*) obj2_ref.val.ref;
  return wasm_i32_value(object_eq(obj1, obj2));
}
