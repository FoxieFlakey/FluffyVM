#ifndef header_1654066032_string_cache_h
#define header_1654066032_string_cache_h

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "fluffyvm.h"
#include "foxgc.h"
#include "collections/queue.h"
#include "ref_counter.h"
#include "util/functional/functional.h"

// String cache with soft references

typedef struct value (*string_cache_string_allocator_t)(struct fluffyvm* vm, const char* str, size_t len, foxgc_root_reference_t** rootRef, void* udata, runnable_t finalizer);
struct string_cache_additional_data {
  pthread_mutex_t onEntryInvalidatedLock;
  pthread_cond_t onEntryInvalidated;
};

struct string_cache_entry {
  

  struct value string;
  foxgc_object_t* gc_this;
  foxgc_object_t* gc_string;
};

struct string_cache {
  pthread_rwlock_t rwlock;

  string_cache_string_allocator_t allocator;
  void* udata;

  struct ref_counter* additionalData;  
  
  struct {
    
  } table;

  foxgc_object_t* gc_this;
};

bool string_cache_init(struct fluffyvm* vm);
void string_cache_cleanup(struct fluffyvm* vm);

struct string_cache* string_cache_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, string_cache_string_allocator_t allocator, void* udata);

// `timeout` is how long this function
// willing to wait until there is expired
// cache entry
// If `timeout `NULL it will not wait
//
// Wait until there is an expired cache
// entry qeued for removal
void string_cache_poll(struct fluffyvm* vm, struct string_cache* cache, struct timespec* timeout);

struct value string_cache_create_string(struct fluffyvm* vm, struct string_cache* this, const char* string, size_t len, foxgc_root_reference_t** rootRef);

#endif

