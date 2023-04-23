#pragma once

#include <stdint.h>
#include "common.h"
#include <stdbool.h>

typedef struct link_item link_item_t;

typedef enum {
  OBJECT,
  BOX_I32,
  BOX_F64
} obj_type_t;

/* HashTable definition */
typedef struct {
  uint32_t cap;
  uint32_t num_elems;
  link_item_t** buckets;
} HashTable;

/* Object definition */
typedef struct {
  obj_type_t type;
  union {
    HashTable ht;
    uint32_t i32;
    double f64;
  } val;
} Object;

struct link_item {
  Object *key;
  Object *val;
  intptr_t hash;
  struct link_item *next;
};



Object* create_object();
uint32_t object_eq(Object* obj1, Object* obj2);
bool is_type(Object* obj, obj_type_t type);
bool is_boxed(Object* obj);

void ht_init(HashTable *ht);
void ht_grow(HashTable *ht);
void ht_insert(HashTable *ht, Object *key, Object *val);
Object* ht_find(HashTable *ht, Object *key);
void ht_destroy(HashTable* ht);
