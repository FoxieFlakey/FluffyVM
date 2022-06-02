#include <stddef.h>

#include "fluffyvm.h"
#include "hashtable.h"
#include "string_cache.h"
#include "fluffyvm_types.h"

#define create_descriptor(name, structure, ...) do { \
  size_t offsets[] = __VA_ARGS__; \
  vm->stringCacheStaticData->name = foxgc_api_descriptor_new(vm->heap, sizeof(offsets) / sizeof(offsets[0]), offsets, sizeof(structure)); \
  if (vm->stringCacheStaticData->name == NULL) \
    return false; \
} while (0)

#define free_descriptor(name) do { \
  if (vm->stringCacheStaticData->name) \
    foxgc_api_descriptor_remove(vm->stringCacheStaticData->name); \
} while(0)


bool string_cache_init(struct fluffyvm* vm) {
  vm->stringCacheStaticData = malloc(sizeof(*vm->stringCacheStaticData));
  if (!vm->stringCacheStaticData)
    return false;
  create_descriptor(desc_string_cache, struct string_cache, {
    offsetof(struct string_cache, gc_this),
    offsetof(struct string_cache, gc_cache)
  });

  return true;
}

void string_cache_cleanup(struct fluffyvm* vm) {
  if (vm->stringCacheStaticData) {
    free_descriptor(desc_string_cache);
    free(vm->stringCacheStaticData);
  }
}

struct string_cache* string_cache_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->stringCacheStaticData->desc_string_cache, NULL);
  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }

  struct string_cache* cache = foxgc_api_object_get_data(obj);
  foxgc_api_write_field(obj, 0, obj);
  
  foxgc_root_reference_t* tmp;
  cache->cache = hashtable_new(vm, 0.75, 16, fluffyvm_get_root(vm), &tmp);
  if (!cache->cache) {
    foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), *rootRef);
    *rootRef = NULL;
    return NULL;
  }

  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmp);

  return NULL;
}

struct value string_cache_create_string(struct fluffyvm* vm, struct string_cache* this, const char* string);
struct value string_cache_create_string2(struct fluffyvm* vm, struct string_cache* this, const char* string, size_t len);

