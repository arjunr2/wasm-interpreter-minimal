#include "ir.h"

void* native_obj_new(void);

void* native_obj_box_i32(uint32_t val);

void* native_obj_box_f64(double val);

void* native_obj_get(void* obj, void* key);

void* native_obj_set(void* obj, void* key, void* val);

uint32_t native_i32_unbox(void* obj);

double native_f64_unbox(void* obj);

uint32_t native_obj_eq(void* obj1, void* obj2);

