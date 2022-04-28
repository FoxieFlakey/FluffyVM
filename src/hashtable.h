#ifndef header_1651049052_hashtable_h
#define header_1651049052_hashtable_h

// Open addressing hash table
// Behaviour is identical to lua's table
// but return FLUFFYVM_NOT_PRESENT when key
// doesn't have any corresponding value

#include "fluffyvm.h"
#include "foxgc.h"
#include "value.h"

struct hashtable {
  struct fluffyvm* vm;

  int capacity;
  int loadFactor;
  int usage;
  foxgc_object_t** table;

  // struct hashtable*
  foxgc_object_t* gc_this;

  // struct key_value_pair***
  foxgc_object_t* gc_table;
};

void hashtable_init(struct fluffyvm* vm);
void hashtable_cleanup(struct fluffyvm* vm);

struct hashtable* hashtable_new(struct fluffyvm* vm, int loadFactor, int initialCapacity, foxgc_root_reference_t** rootRef);
// False if there an error
bool hashtable_set(struct fluffyvm* vm, struct hashtable* this, struct value key, struct value value);
struct value hashtable_get(struct fluffyvm* vm, struct hashtable* this, struct value key);

#endif

