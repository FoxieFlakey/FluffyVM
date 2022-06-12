#include <Block.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "hashtable.h"
#include "fluffyvm.h"
#include "fluffyvm_types.h"
#include "value.h"
#include "hashing.h"

struct hashtable_pair {
  struct value key;
  struct value value;
  
  struct hashtable_pair* next;

  // struct key_value_pair*
  foxgc_object_t* gc_this; 
  foxgc_object_t* gc_next;
  foxgc_object_t* gc_key;
  foxgc_object_t* gc_value;
};

#define UNIQUE_KEY(name) static uintptr_t name = (uintptr_t) &name

UNIQUE_KEY(hashtableTypeKey);
UNIQUE_KEY(pairTypeKey);
UNIQUE_KEY(tableArrayTypeKey);

#define create_descriptor(name2, key, name, structure, ...) do { \
  foxgc_descriptor_pointer_t offsets[] = __VA_ARGS__; \
  vm->hashTableStaticData->name = foxgc_api_descriptor_new(vm->heap, fluffyvm_get_owner_key(), key, name2, sizeof(offsets) / sizeof(offsets[0]), offsets, sizeof(structure)); \
  if (vm->hashTableStaticData->name == NULL) \
    return false; \
} while (0)

#define free_descriptor(name) do { \
  if (vm->hashTableStaticData->name) \
    foxgc_api_descriptor_remove(vm->hashTableStaticData->name); \
} while(0)

#define PAIR_OFFSET_THIS (0)
#define PAIR_OFFSET_NEXT (1)
#define PAIR_OFFSET_KEY (2)
#define PAIR_OFFSET_VALUE (3)

#define HASHTABLE_OFFSET_THIS (0)
#define HASHTABLE_OFFSET_TABLE (1)

bool hashtable_init(struct fluffyvm* vm) {
  vm->hashTableStaticData = malloc(sizeof(*vm->hashTableStaticData));
  if (!vm->hashTableStaticData)
    return false;
  
  create_descriptor("net.fluffyfox.fluffyvm.hashtable.HashTable", hashtableTypeKey, desc_hashTable, struct hashtable, {
    {"this",  offsetof(struct hashtable, gc_this)},
    {"table", offsetof(struct hashtable, gc_table)}
  });

  create_descriptor("net.fluffyfox.fluffyvm.hashtable.Pair", pairTypeKey, desc_pair, struct hashtable_pair, {
    {"this", offsetof(struct hashtable_pair, gc_this)},
    {"next", offsetof(struct hashtable_pair, gc_next)},
    {"key", offsetof(struct hashtable_pair, gc_key)},
    {"value", offsetof(struct hashtable_pair, gc_value)}
  });

  return true;
}

void hashtable_cleanup(struct fluffyvm* vm) {
  if (!vm->hashTableStaticData)
    return;

  free_descriptor(desc_hashTable);
  free_descriptor(desc_pair);
  free(vm->hashTableStaticData);
}

////////////////////////////////////////////
// struct key_value_pair* management

static inline void pair_write_next(struct hashtable_pair* this, struct hashtable_pair* pair) {
  foxgc_api_write_field(this->gc_this, PAIR_OFFSET_NEXT, pair->next->gc_this);
  this->next = pair;
}

static inline void pair_write_key(struct hashtable_pair* this, struct value value) {
  foxgc_object_t* ptr;
  if ((ptr = value_get_object_ptr(value)))
    foxgc_api_write_field(this->gc_this, PAIR_OFFSET_KEY, ptr);
  value_copy(&this->key, value);
}

static inline void pair_write_value(struct hashtable_pair* this, struct value value) {
  foxgc_object_t* ptr;
  if ((ptr = value_get_object_ptr(value)))
    foxgc_api_write_field(this->gc_this, PAIR_OFFSET_VALUE, ptr);
  value_copy(&this->value, value);
}

static struct hashtable_pair* new_pair(struct fluffyvm* vm, foxgc_root_reference_t** rootRef) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, NULL, fluffyvm_get_root(vm), rootRef, vm->hashTableStaticData->desc_pair, NULL);
  if (!obj) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL; 
  }

  struct hashtable_pair* pair = foxgc_api_object_get_data(obj);
  foxgc_api_write_field(obj, PAIR_OFFSET_THIS, obj);
  foxgc_api_write_field(obj, PAIR_OFFSET_NEXT, NULL);
  foxgc_api_write_field(obj, PAIR_OFFSET_KEY, NULL);
  foxgc_api_write_field(obj, PAIR_OFFSET_VALUE, NULL);
  pair->next = NULL;
  value_copy(&pair->key, value_not_present());
  value_copy(&pair->value, value_not_present());
  
  return pair;
}

////////////////////////////////////////////

static inline void hashtable_write_table(struct hashtable* this, foxgc_object_t* obj) {
  foxgc_api_write_field(this->gc_this, HASHTABLE_OFFSET_TABLE, obj);
  this->table = foxgc_api_object_get_data(obj);
}

static bool set_entry(struct fluffyvm* vm, foxgc_object_t* tableObj, foxgc_object_t** table, int capacity, struct value key, struct value value) {
  uint64_t hash = -1;
  if (!value_hash_code(key, &hash)) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.badKey);
    return false;
  }
  uint64_t index = hash & (capacity - 1);

  // Try insert
  struct hashtable_pair* prev = NULL;
  struct hashtable_pair* current = table[index] ? foxgc_api_object_get_data(table[index]) : NULL;
  while (current) {
    prev = current;
    uint64_t hash2 = -1;
    bool res = value_hash_code(current->key, &hash2);
    assert(res); /* Cannot happen unless there is bug
                    in the value_hash_code function */

    // Overwrite existing if there is match
    if (hash == hash2 && value_equals(current->key, key)) { 
      pair_write_key(current, key); 
      pair_write_value(current, value); 
      return true;
    }
    current = prev->next;
  }
  
  foxgc_root_reference_t* rootRef = NULL;
  struct hashtable_pair* pair = new_pair(vm, &rootRef);
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
  if (!(desiredCapacity && (!(desiredCapacity & (desiredCapacity - 1))))) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.invalidCapacity);
    return false;
  }

  foxgc_root_reference_t* newTableRootRef = NULL;

  foxgc_object_t* oldTable = this->gc_table;
  foxgc_object_t* newTable = foxgc_api_new_array(vm->heap, fluffyvm_get_owner_key(), tableArrayTypeKey, NULL, fluffyvm_get_root(vm), &newTableRootRef, desiredCapacity, NULL);
  if (!newTable) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return false;
  }
  
  // Rehash
  if (oldTable) {   
    for (int i = 0; i < prevCapacity; i++) {
      foxgc_object_t* nextObj = this->table[i];
      struct hashtable_pair* next = nextObj ? foxgc_api_object_get_data(nextObj) : NULL;
      struct hashtable_pair* currentPair = NULL;
      
      while (next) {
        currentPair = next;
        next = next->next;
        if (!set_entry(vm, newTable, foxgc_api_object_get_data(newTable), desiredCapacity, currentPair->key, currentPair->value)) {
          foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), newTableRootRef);
          return false;
        }
      }
    }
  }

  this->capacity = desiredCapacity;
  hashtable_write_table(this, newTable);
  
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), newTableRootRef);
  return true;
}

struct hashtable* hashtable_new(struct fluffyvm* vm, double loadFactor, int initialCapacity, foxgc_root_t* root, foxgc_root_reference_t** rootRef) {
  if (!(initialCapacity > 0 && (!(initialCapacity & (initialCapacity - 1))))) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.invalidCapacity);
    return NULL;
  }

  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, NULL, root, rootRef, vm->hashTableStaticData->desc_hashTable, ^void (foxgc_object_t* obj) {
    struct hashtable* this = foxgc_api_object_get_data(obj);  
    pthread_rwlock_destroy(&this->lock);
  });
  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }

  struct hashtable* this = foxgc_api_object_get_data(obj);
  foxgc_api_write_field(obj, HASHTABLE_OFFSET_THIS, obj);
  foxgc_api_write_field(obj, HASHTABLE_OFFSET_TABLE, NULL);

  this->capacity = 0;
  this->usage = 0;
  this->loadFactor = loadFactor;
  this->table = NULL;
  pthread_rwlock_init(&this->lock, NULL);
  
  if (resize(vm, this, 0, initialCapacity) == false) {
    foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), *rootRef);
    *rootRef = NULL;
    return NULL;
  }

  return this;
}

bool hashtable_set(struct fluffyvm* vm, struct hashtable* this, struct value key, struct value value) {
  if (key.type == FLUFFYVM_TVALUE_NOT_PRESENT ||
      value.type == FLUFFYVM_TVALUE_NOT_PRESENT) {
    return false;
  }

  pthread_rwlock_wrlock(&this->lock);
  if (this->usage + 1 >= (this->capacity * this->loadFactor) / 100) {
    if (resize(vm, this, this->capacity, this->capacity * 2) == false) {
      pthread_rwlock_unlock(&this->lock);
      return false;
    }
  }
  
  set_entry(vm, this->gc_table, this->table, this->capacity, key, value);
  this->usage++;

  pthread_rwlock_unlock(&this->lock);
  return true;
}

typedef bool (^equal_operation)(struct value key);
static struct value internal_get(struct fluffyvm* vm, struct hashtable* this, uint64_t hash, foxgc_root_reference_t** rootRef, equal_operation equals) {
  pthread_rwlock_rdlock(&this->lock);
  uint64_t index = hash & (this->capacity - 1);
 
  struct hashtable_pair* current = this->table[index] ? foxgc_api_object_get_data(this->table[index]) : NULL;
  while (current) {
    uint64_t hash2 = -1;
    bool res = value_hash_code(current->key, &hash2);
    assert(res); /* Cannot happen unless there is bug
                    in the value_hash_code function */
    
    if (hash == hash2 && equals(current->key))
      break;

    current = current->next;
  }
 
  if (current && value_get_object_ptr(current->value))
    foxgc_api_root_add(vm->heap, value_get_object_ptr(current->value), fluffyvm_get_root(vm), rootRef);
  
  struct value result = current ? current->value : value_not_present();
  pthread_rwlock_unlock(&this->lock);

  return result;
}

struct value hashtable_get(struct fluffyvm* vm, struct hashtable* this, struct value key, foxgc_root_reference_t** rootRef) {
  uint64_t hash = -1;
  if (!value_hash_code(key, &hash)) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.badKey);
    return value_not_present();
  }
  
  return internal_get(vm, this, hash, rootRef, ^bool (struct value op2) {
    return value_equals(key, op2);
  });
}

void hashtable_remove(struct fluffyvm* vm, struct hashtable* this, struct value key) {
  uint64_t hash = -1;
  if (!value_hash_code(key, &hash))
    return;
  
  pthread_rwlock_wrlock(&this->lock);
  uint64_t index = hash & (this->capacity - 1);
 
  struct hashtable_pair* current = this->table[index] ? foxgc_api_object_get_data(this->table[index]) : NULL;
  struct hashtable_pair* prev = NULL; 
  while (current) {
    uint64_t hash2 = -1;
    bool res = value_hash_code(current->key, &hash2);
    assert(res); /* Cannot happen unless there is bug
                    in the value_hash_code function */
    
    if (hash == hash2 && value_equals(current->key, key)) {
      if (prev)
        pair_write_next(prev, NULL);
      else
        foxgc_api_write_array(this->gc_table, index, NULL);
      goto quit_function;
    }

    prev = current;
    current = current->next;
  }

  quit_function:
  pthread_rwlock_unlock(&this->lock);
}

struct value hashtable_get2(struct fluffyvm* vm, struct hashtable* this, const char* key, size_t len, foxgc_root_reference_t** rootRef) {
  uint64_t hash = hashing_hash_default(key, len); 

  return internal_get(vm, this, hash, rootRef, ^bool (struct value op2) {
    if (op2.type != FLUFFYVM_TVALUE_STRING)
      return false;
    size_t compareLen = foxgc_api_get_array_length(op2.data.str->str);
    if (compareLen > len)
      compareLen = len;
    return memcmp(key, foxgc_api_object_get_data(op2.data.str->str), compareLen);
  });
}

struct value hashtable_next(struct fluffyvm* vm, struct hashtable* this, struct value key) {  
  pthread_rwlock_rdlock(&this->lock);
  struct value newKey = value_not_present();
  bool hasFound = false;
  int bucketStart = 0; 
  uint64_t hash = -1;
  
  if (key.type != FLUFFYVM_TVALUE_NOT_PRESENT) {
    if (!value_hash_code(key, &hash))
      return value_not_present();
    bucketStart = hash & (this->capacity - 1);
  }

  for (int bucket = bucketStart; bucket < this->capacity; bucket++) {
    if (!this->table[bucket])
      continue;

    struct hashtable_pair* pair = this->table[bucket] ? foxgc_api_object_get_data(this->table[bucket]) : NULL;
    if (pair && key.type == FLUFFYVM_TVALUE_NOT_PRESENT) {
      value_copy(&newKey, pair->key);
      break;
    }

    while (pair && !hasFound) {
      if (value_equals(pair->key, key))
        hasFound = true; 
      pair = pair->next;
    }
 
    if (pair) {
      value_copy(&newKey, pair->key);
      break;
    } 
  } 
  
  pthread_rwlock_unlock(&this->lock);
  return newKey;
}







