#include <Block.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "hashtable.h"
#include "fluffyvm.h"
#include "fluffyvm_types.h"
#include "value.h"

struct pair {
  struct value key;
  struct value value;
  
  struct pair* next;

  // struct key_value_pair*
  foxgc_object_t* gc_this; 
  foxgc_object_t* gc_next;
  foxgc_object_t* gc_key;
  foxgc_object_t* gc_value;
};

#define create_descriptor(vm, name, structure, ...) do { \
  size_t offsets[] = __VA_ARGS__; \
  vm->hashTableStaticData->name = foxgc_api_descriptor_new(vm->heap, sizeof(offsets) / sizeof(offsets[0]), offsets, sizeof(structure)); \
} while (0)

#define PAIR_OFFSET_THIS (0)
#define PAIR_OFFSET_NEXT (1)
#define PAIR_OFFSET_KEY (2)
#define PAIR_OFFSET_VALUE (3)

#define HASHTABLE_OFFSET_THIS (0)
#define HASHTABLE_OFFSET_TABLE (1)


void hashtable_init(struct fluffyvm* vm) {
  vm->hashTableStaticData = malloc(sizeof(*vm->hashTableStaticData));
  create_descriptor(vm, desc_hashTable, struct hashtable, {
    offsetof(struct hashtable, gc_this),
    offsetof(struct hashtable, gc_table)
  });

  create_descriptor(vm, desc_pair, struct pair, {
    offsetof(struct pair, gc_this),
    offsetof(struct pair, gc_next),
    offsetof(struct pair, gc_key),
    offsetof(struct pair, gc_value)
  });
}

void hashtable_cleanup(struct fluffyvm* vm) {
  foxgc_api_descriptor_remove(vm->hashTableStaticData->desc_hashTable);
  foxgc_api_descriptor_remove(vm->hashTableStaticData->desc_pair);
  free(vm->hashTableStaticData);
}

////////////////////////////////////////////
// struct key_value_pair* management

static inline void pair_write_next(struct pair* this, struct pair* pair) {
  foxgc_api_write_field(this->gc_this, PAIR_OFFSET_NEXT, pair->next->gc_this);
  this->next = pair;
}

static inline void pair_write_key(struct pair* this, struct value value) {
  foxgc_object_t* ptr;
  if ((ptr = value_get_object_ptr(value)))
    foxgc_api_write_field(this->gc_this, PAIR_OFFSET_KEY, ptr);
  value_copy(&this->key, &value);
}

static inline void pair_write_value(struct pair* this, struct value value) {
  foxgc_object_t* ptr;
  if ((ptr = value_get_object_ptr(value)))
    foxgc_api_write_field(this->gc_this, PAIR_OFFSET_VALUE, ptr);
  value_copy(&this->value, &value);
}

static struct pair* new_pair(struct fluffyvm* vm, foxgc_root_reference_t** rootRef) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->hashTableStaticData->desc_pair, NULL);
  if (!obj)
    return NULL; 
  
  struct pair* pair = foxgc_api_object_get_data(obj);
  foxgc_api_write_field(obj, PAIR_OFFSET_THIS, obj);
  foxgc_api_write_field(obj, PAIR_OFFSET_NEXT, NULL);
  foxgc_api_write_field(obj, PAIR_OFFSET_KEY, NULL);
  foxgc_api_write_field(obj, PAIR_OFFSET_VALUE, NULL);
  pair->next = NULL;
  struct value tmp = value_not_present();
  value_copy(&pair->key, &tmp);
  value_copy(&pair->value, &tmp);
  
  return pair;
}

////////////////////////////////////////////

static inline void hashtable_write_table(struct hashtable* this, foxgc_object_t* obj) {
  foxgc_api_write_field(this->gc_this, HASHTABLE_OFFSET_TABLE, obj);
  this->table = foxgc_api_object_get_data(obj);
}

static bool set_entry(struct fluffyvm* vm, foxgc_object_t* tableObj, foxgc_object_t** table, int capacity, struct value key, struct value value) {
  uintptr_t hash = -1;
  bool res = value_hash_code(key, &hash);
  assert(res); /* Bad key */
  int index = hash & (capacity - 1);

  // Try insert
  struct pair* prev = NULL;
  struct pair* current = table[index] ? foxgc_api_object_get_data(table[index]) : NULL;
  while (current) {
    prev = current;
    uintptr_t hash2 = -1;
    value_hash_code(current->key, &hash2);
    
    // Overwrite existing if there is match
    if (hash == hash2 && value_equals(current->key, key)) { 
      pair_write_key(current, key); 
      pair_write_value(current, value); 
      return true;

      //break;
    }
    current = prev->next;
  }
  
  foxgc_root_reference_t* rootRef = NULL;
  struct pair* pair = new_pair(vm, &rootRef);
  if (!pair)
    return false;

  pair_write_key(pair, key); 
  pair_write_value(pair, value); 
  
  if (prev)
    pair_write_next(prev, pair);
  else
    foxgc_api_write_array(tableObj, index, pair->gc_this);
 
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), rootRef);
  return true;
}

static bool resize(struct fluffyvm* vm, struct hashtable* this, int prevCapacity, int desiredCapacity) {
  assert(desiredCapacity && (!(desiredCapacity & (desiredCapacity - 1))));
  foxgc_root_reference_t* newTableRootRef = NULL;

  foxgc_object_t* oldTable = this->gc_table;
  foxgc_object_t* newTable = foxgc_api_new_array(vm->heap, fluffyvm_get_root(vm), &newTableRootRef, desiredCapacity, NULL);
  if (!newTable)
    return false;
  
  // Rehash
  if (oldTable) {   
    for (int i = 0; i < prevCapacity; i++) {
      foxgc_object_t* nextObj = this->table[i];
      struct pair* next = nextObj ? foxgc_api_object_get_data(nextObj) : NULL;
      struct pair* currentPair = NULL;
      
      while (next) {
        currentPair = next;
        next = next->next;
        if (!set_entry(vm, newTable, foxgc_api_object_get_data(newTable), desiredCapacity, currentPair->key, currentPair->value)) {
          goto no_memory;
        }
      }
    }
  }

  this->capacity = desiredCapacity;
  hashtable_write_table(this, newTable);
  
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), newTableRootRef);
  return true;
  
  // Revert if failed due no memory
  no_memory:
  hashtable_write_table(this, oldTable);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), newTableRootRef);
  return false;
}

struct hashtable* hashtable_new(struct fluffyvm* vm, int loadFactor, int initialCapacity, foxgc_root_t* root, foxgc_root_reference_t** rootRef) {
  if (!(initialCapacity > 0 && (!(initialCapacity & (initialCapacity - 1))))) {
    return NULL;    
  }

  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, root, rootRef, vm->hashTableStaticData->desc_hashTable, NULL);
  struct hashtable* this = foxgc_api_object_get_data(obj);
  foxgc_api_write_field(obj, HASHTABLE_OFFSET_THIS, obj);
  foxgc_api_write_field(obj, HASHTABLE_OFFSET_TABLE, NULL);

  this->capacity = 0;
  this->usage = 0;
  this->loadFactor = loadFactor;
  this->table = NULL;
  
  if (resize(vm, this, 0, initialCapacity) == false)
    goto no_memory; 

  return this;
  no_memory:
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), *rootRef);
  *rootRef = NULL;
  return NULL;
}

bool hashtable_set(struct fluffyvm* vm, struct hashtable* this, struct value key, struct value value) {
  if (key.type == FLUFFYVM_NOT_PRESENT ||
      value.type == FLUFFYVM_NOT_PRESENT) {
    return false;
  }

  if (this->usage + 1 >= (this->capacity * this->loadFactor) / 100) {
    if (resize(vm, this, this->capacity, this->capacity * 2) == false) {
      return false;
    }
  }
  
  set_entry(vm, this->gc_table, this->table, this->capacity, key, value);
  this->usage++;

  return true;
}

struct value hashtable_get(struct fluffyvm* vm, struct hashtable* this, struct value key, foxgc_root_reference_t** rootRef) {
  uintptr_t hash = -1;
  bool res = value_hash_code(key, &hash);
  assert(res); /* Bad key */
  int index = hash & (this->capacity - 1);
 
  struct pair* current = this->table[index] ? foxgc_api_object_get_data(this->table[index]) : NULL;
  while (current) {
    uintptr_t hash2 = -1;
    value_hash_code(current->key, &hash2);
    if (hash == hash2 && value_equals(current->key, key))
       break;

    current = current->next;
  }
 
  if (current && value_get_object_ptr(current->value))
    foxgc_api_root_add(vm->heap, value_get_object_ptr(current->value), fluffyvm_get_root(vm), rootRef);
  return current ? current->value : value_not_present();
}
