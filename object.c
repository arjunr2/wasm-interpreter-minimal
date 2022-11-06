#include "object.h"
#include <stdlib.h>
#include <stdio.h>

#define INITIAL_CAP 16
#define LOAD_FACTOR 60

static intptr_t hash64shift(intptr_t key)
{
  key = (~key) + (key << 21); // key = (key << 21) - key - 1;
  key = key ^ (key >> 24);
  key = (key + (key << 3)) + (key << 8); // key * 265
  key = key ^ (key >> 14);
  key = (key + (key << 2)) + (key << 4); // key * 21
  key = key ^ (key >> 28);
  key = key + (key << 31);
  return key;
}


/* Create an object with specific type; data uninitialized */
Object* create_object(obj_type_t type) {
  Object* obj = (Object*) malloc(sizeof(Object));
  obj->type = type;
  return obj;
}

/* Check equality of objects */
uint32_t object_eq(Object* obj1, Object* obj2) {
  if (obj1->type != obj2->type) {
    return 0;
  }
  switch (obj1->type) {
    case OBJECT: return obj1 == obj2; break;
    case BOX_I32: return (obj1->val.i32 == obj2->val.i32); break;
    case BOX_F64: return (obj1->val.f64 == obj2->val.f64); break;
    default:
      ERR("Error type for object equality; unreachable\n");
  }
}


/* HashTable methods */

void ht_init(HashTable *ht) {
  ht->cap = INITIAL_CAP;
  ht->num_elems = 0;
  ht->buckets = (link_item_t**) calloc(ht->cap, sizeof(link_item_t*));
}


// TODO
void ht_grow(HashTable *ht) {
  uint32_t new_cap = ht->cap * 2;
  link_item_t** new_buckets = (link_item_t**) calloc(new_cap, sizeof(link_item_t*));
  // Rehash all elements from old buckets
}

// TODO
void ht_insert(HashTable *ht, Object *key, Object *val) {
  return;
}

// TODO
Object* ht_find(HashTable *ht, Object *key) {
  return NULL;
}


// TODO
void ht_destroy(HashTable* ht) {
  for (uint32_t i = 0; i < ht->cap; ht++) {
    link_item_t* head = ht->buckets[i];
    while (head != NULL) {
      // Recursively destroy objects
      if (head->obj.type == OBJECT) { ht_destroy(&head->obj.val.ht); }
      link_item_t* old = head;
      head = head->next;
      free(old);
    }
  }
  free(ht->buckets);
}
