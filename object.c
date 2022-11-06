#include "object.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_CAP 16
#define LOAD_FACTOR 60

static intptr_t hash_object(Object *obj) {
  intptr_t key;
  switch(obj->type) {
    case OBJECT:  key = (intptr_t) obj; break;
    case BOX_I32: key = obj->val.i32; break;
    case BOX_F64: memcpy(&key, &obj->val.f64, sizeof(intptr_t)); break;
  }

  key = (~key) + (key << 21); // key = (key << 21) - key - 1;
  key = key ^ (key >> 24);
  key = (key + (key << 3)) + (key << 8); // key * 265
  key = key ^ (key >> 14);
  key = (key + (key << 2)) + (key << 4); // key * 21
  key = key ^ (key >> 28);
  key = key + (key << 31);
  return key;
}

static link_item_t* ht_find_internal(HashTable *ht, Object *key) {
  intptr_t hash = hash_object(key);
  /* Note: Only power of 2 capacities */
  uint32_t hash_idx = hash & (ht->cap - 1);

  link_item_t* head = ht->buckets[hash_idx];
  while (head != NULL) {
    if (head->hash == hash) {
      if (object_eq(head->key, key)) { return head; }
    }
    head = head->next;
  }
  return NULL;
}

static void ht_insert_internal(link_item_t **buckets, Object *key, Object *val, uint32_t cap) {
  intptr_t hash = hash_object(key);
  /* Note: Only power of 2 capacities */
  uint32_t hash_idx = hash & (cap - 1);

  link_item_t **bucket = buckets + hash_idx;

  link_item_t* new_elem = (link_item_t*) malloc(sizeof(link_item_t));
  new_elem->next = *bucket;
  *bucket = new_elem;

  new_elem->hash = hash;
  new_elem->key = key;
  new_elem->val = val;
}


/* Create an object with specific type; data uninitialized */
Object* create_object(obj_type_t type) {
  Object* obj = (Object*) malloc(sizeof(Object));
  obj->type = type;
  return obj;
}

/* Check equality of objects */
uint32_t object_eq(Object* obj1, Object* obj2) {
  if ((obj1 == NULL) || (obj2 == NULL)) { return obj1 == obj2; }
  if (obj1->type != obj2->type) { return 0; }
  switch (obj1->type) {
    case OBJECT: return obj1 == obj2; break;
    case BOX_I32: return (obj1->val.i32 == obj2->val.i32); break;
    case BOX_F64: return (obj1->val.f64 == obj2->val.f64); break;
    default:
      ERR("Error type for object equality; unreachable\n");
  }
}

bool is_type(Object* obj, obj_type_t type) {
  return (obj->type == type);
}

bool is_boxed(Object* obj) {
  return (obj->type != OBJECT);
}

/* HashTable methods */

void ht_init(HashTable *ht) {
  ht->cap = INITIAL_CAP;
  ht->num_elems = 0;
  ht->buckets = (link_item_t**) calloc(ht->cap, sizeof(link_item_t*));
}


void ht_grow(HashTable *ht) {
  uint32_t new_cap = ht->cap * 2;
  link_item_t** new_buckets = (link_item_t**) calloc(new_cap, sizeof(link_item_t*));
  // Rehash all elements from old buckets
  for (uint32_t i = 0; i < ht->cap; i++) {
    link_item_t *head = ht->buckets[i];
    while (head != NULL) {
      intptr_t hash = hash_object(head->key);
      ht_insert_internal(new_buckets, head->key, head->val, new_cap);
      head = head->next;
    }
  }

  ht->buckets = new_buckets;
  ht->cap = new_cap;
}


void ht_insert(HashTable *ht, Object *key, Object *val) {
  link_item_t *result = ht_find_internal(ht, key);

  if (result != NULL) {
    /* Overwrite object */
    result->val = val;
  } else {
    if (((ht->num_elems*100)/ht->cap) > LOAD_FACTOR) { ht_grow(ht); }
    /* Insert new element */
    ht_insert_internal(ht->buckets, key, val, ht->cap);
    ht->num_elems++;
  }
}


Object* ht_find(HashTable *ht, Object *key) {
  intptr_t hash;
  link_item_t *result = ht_find_internal(ht, key);
  if (result != NULL) {
    return result->val;
  }
  return NULL;
}


// TODO
void ht_destroy(HashTable* ht) {
  for (uint32_t i = 0; i < ht->cap; ht++) {
    link_item_t* head = ht->buckets[i];
    while (head != NULL) {
      // Recursively destroy objects
      if (head->val->type == OBJECT) { ht_destroy(&head->val->val.ht); }
      link_item_t* old = head;
      head = head->next;
      free(old);
    }
  }
  free(ht->buckets);
}
