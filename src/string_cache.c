#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include "fluffyvm.h"
#include "hashtable.h"
#include "string_cache.h"
#include "fluffyvm_types.h"
#include "value.h"

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

struct cache_entry {
  struct value string;
  foxgc_soft_reference_t* softReference;
  
  foxgc_object_t* gc_this;
  foxgc_object_t* gc_softReference;
};

bool string_cache_init(struct fluffyvm* vm) {
  vm->stringCacheStaticData = malloc(sizeof(*vm->stringCacheStaticData));
  if (!vm->stringCacheStaticData)
    return false;
  create_descriptor(desc_string_cache, struct string_cache, {
    offsetof(struct string_cache, gc_this),
    offsetof(struct string_cache, gc_cache)
  });
  
  create_descriptor(desc_string_cache_entry, struct cache_entry, {
    offsetof(struct cache_entry, gc_this),
    offsetof(struct cache_entry, gc_softReference)
  });
  
  vm->modules.stringCache.moduleID = value_get_module_id();
  vm->modules.stringCache.type.softReference = 1;

  return true;
}

void string_cache_cleanup(struct fluffyvm* vm) {
  if (vm->stringCacheStaticData) {
    free_descriptor(desc_string_cache);
    free_descriptor(desc_string_cache_entry);
    free(vm->stringCacheStaticData);
  }
}

struct string_cache* string_cache_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, string_cache_string_allocator_t allocator, void* udata) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->stringCacheStaticData->desc_string_cache, NULL);
  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }

  struct string_cache* cache = foxgc_api_object_get_data(obj);
  
  foxgc_root_reference_t* tmp = NULL;
  cache->cache = hashtable_new(vm, 0.75, 1024, fluffyvm_get_root(vm), &tmp); 
  cache->allocator = allocator;
  cache->udata = udata;

  if (!cache->cache) {
    foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), *rootRef);
    *rootRef = NULL;
    return NULL;
  } 
  foxgc_api_write_field(obj, 1, cache->cache->gc_this);
  foxgc_api_write_field(obj, 0, obj);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmp);

  return cache;
}

struct value string_cache_create_string(struct fluffyvm* vm, struct string_cache* this, const char* string, size_t len, foxgc_root_reference_t** rootRef) {
  // Check if string exist in cache
  struct value cachedStringEntry = hashtable_get2(vm, this->cache, string, len, rootRef);
  if (cachedStringEntry.type != FLUFFYVM_TVALUE_NOT_PRESENT) {   
    assert(cachedStringEntry.type == FLUFFYVM_TVALUE_GARBAGE_COLLECTABLE_USERDATA);
    struct cache_entry* entry = foxgc_api_object_get_data(cachedStringEntry.data.userdata->userGarbageCollectableData);
    foxgc_object_t* obj = foxgc_api_reference_get(entry->softReference, fluffyvm_get_root(vm), rootRef);
    if (!obj)
      goto cache_miss;
    return entry->string;
  }

  cache_miss:;
  struct value newString = this->allocator(vm, string, len, rootRef, this->udata);
  if (newString.type == FLUFFYVM_TVALUE_NOT_PRESENT)
    return value_not_present();
  
  foxgc_root_reference_t* tmp = NULL;
  foxgc_object_t* cacheEntry = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), &tmp, vm->stringCacheStaticData->desc_string_cache_entry, NULL);
  if (!cacheEntry)
    goto try_later;
  struct cache_entry* entry = foxgc_api_object_get_data(cacheEntry);
  value_copy(&entry->string, newString);
  foxgc_api_write_field(cacheEntry, 0, cacheEntry);

  foxgc_root_reference_t* tmp2 = NULL;
  foxgc_soft_reference_t* reference = foxgc_api_new_soft_reference(vm->heap, fluffyvm_get_root(vm), &tmp2, value_get_object_ptr(newString));
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmp);
  
  if (reference == NULL) 
    goto try_later;
  
  foxgc_api_write_field(cacheEntry, 1, foxgc_api_reference_get_reference_object(reference));
  entry->softReference = reference;

  foxgc_root_reference_t* tmp3 = NULL; 
  struct value entryValue = value_new_garbage_collectable_userdata(vm, vm->modules.stringCache.moduleID, vm->modules.stringCache.type.softReference, cacheEntry, &tmp3);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmp2);
  
  if (entryValue.type == FLUFFYVM_TVALUE_NOT_PRESENT)
    goto try_later;
  
  // Its fine if this failed to set
  // we can try add it to cache later
  hashtable_set(vm, this->cache, newString, entryValue);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmp3);
  
  try_later:
  return newString;
}





