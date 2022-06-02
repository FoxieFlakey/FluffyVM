#ifndef header_1651049052_hashtable_h
#define header_1651049052_hashtable_h

// Open addressing hash table
// Behaviour is identical to lua's table
// but return FLUFFYVM_TVALUE_NOT_PRESENT when key
// doesn't have any corresponding value

#include <pthread.h>
#include <stdbool.h>

#include "api_layer/types.h"
#include "fluffyvm.h"
#include "foxgc.h"
#include "value.h"

struct hashtable {
  pthread_rwlock_t lock;
  struct fluffyvm* vm;

  int capacity;
  double loadFactor;
  int usage;
  foxgc_object_t** table;

  // struct hashtable*
  foxgc_object_t* gc_this;

  // struct key_value_pair***
  foxgc_object_t* gc_table;
};

bool hashtable_init(struct fluffyvm* vm);
void hashtable_cleanup(struct fluffyvm* vm);

struct hashtable* hashtable_new(struct fluffyvm* vm, double loadFactor, int initialCapacity, foxgc_root_t* root, foxgc_root_reference_t** rootRef);
// False if there an error
bool hashtable_set(struct fluffyvm* vm, struct hashtable* this, struct value key, struct value value);
struct value hashtable_get(struct fluffyvm* vm, struct hashtable* this, struct value key, foxgc_root_reference_t** rootRef);

// Able to take string directly
struct value hashtable_get2(struct fluffyvm* vm, struct hashtable* this, const char* key, size_t len, foxgc_root_reference_t** rootRef);

// Remove key value pair
void hashtable_remove(struct fluffyvm* vm, struct hashtable* this, struct value key);

// Pretty much like `next` in lua
// But return next key instead value
// Return not present if there no next element
struct value hashtable_next(struct fluffyvm* vm, struct hashtable* this, struct value key);

#endif






