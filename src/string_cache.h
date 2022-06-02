#ifndef header_1654066032_string_cache_h
#define header_1654066032_string_cache_h

#include "fluffyvm.h"
#include "foxgc.h"
#include "hashtable.h"

// String cache with soft references

typedef struct value (*string_cache_string_allocator_t)(struct fluffyvm* vm, const char* str, size_t len, foxgc_root_reference_t** rootRef, void* udata);
struct string_cache {
  struct hashtable* cache;
  string_cache_string_allocator_t allocator;
  void* udata;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_cache;
};

bool string_cache_init(struct fluffyvm* vm);
void string_cache_cleanup(struct fluffyvm* vm);

struct string_cache* string_cache_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, string_cache_string_allocator_t allocator, void* udata);

void string_cache_poll(struct fluffyvm* vm, struct string_cache* cache);

struct value string_cache_create_string(struct fluffyvm* vm, struct string_cache* this, const char* string, size_t len, foxgc_root_reference_t** rootRef);

#endif

