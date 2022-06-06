#include <pthread.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <time.h>
#include <Block.h>

#include "fluffyvm.h"
#include "hashtable.h"
#include "ref_counter.h"
#include "string_cache.h"
#include "fluffyvm_types.h"
#include "value.h"
#include "config.h"

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
  foxgc_object_t* gc_this;
  foxgc_object_t* gc_string;
};

bool string_cache_init(struct fluffyvm* vm) {
  vm->stringCacheStaticData = malloc(sizeof(*vm->stringCacheStaticData));
  if (!vm->stringCacheStaticData)
    return false;
  create_descriptor(desc_string_cache, struct string_cache, {
    offsetof(struct string_cache, gc_this)
  });
  
  create_descriptor(desc_string_cache_entry, struct cache_entry, {
    offsetof(struct cache_entry, gc_this),
    offsetof(struct cache_entry, gc_string)
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
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->stringCacheStaticData->desc_string_cache, Block_copy(^void (foxgc_object_t* obj) {
    struct string_cache* cache = foxgc_api_object_get_data(obj);
    pthread_rwlock_destroy(&cache->rwlock);
    ref_counter_dec(cache->additionalData);
  }));

  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }

  struct string_cache* cache = foxgc_api_object_get_data(obj);
  struct string_cache_additional_data* additionalData = malloc(sizeof(*additionalData));
  pthread_mutex_init(&additionalData->onEntryInvalidatedLock, NULL);
  pthread_cond_init(&additionalData->onEntryInvalidated, NULL);
  pthread_rwlock_init(&cache->rwlock, NULL);

  cache->allocator = allocator;
  cache->udata = udata;
  cache->additionalData = ref_counter_new(additionalData, Block_copy(^void (struct ref_counter* counter) {
    pthread_mutex_destroy(&additionalData->onEntryInvalidatedLock);
    pthread_cond_destroy(&additionalData->onEntryInvalidated);
    free(additionalData);
  })); 
  foxgc_api_write_field(obj, 0, obj);

  return cache;
}

struct value string_cache_create_string(struct fluffyvm* vm, struct string_cache* this, const char* string, size_t len, foxgc_root_reference_t** rootRef) {
  return this->allocator(vm, string, len, rootRef, this->udata, NULL);
  
  /*
  // Check if string exist in cache
  pthread_rwlock_rdlock(&this->rwlock);
  struct value cachedStringEntry = getCachedItem(vm, this, string, len, rootRef);
  pthread_rwlock_unlock(&this->rwlock);
  
  if (cachedStringEntry.type != FLUFFYVM_TVALUE_NOT_PRESENT) {   
    assert(cachedStringEntry.type == FLUFFYVM_TVALUE_GARBAGE_COLLECTABLE_USERDATA);
    foxgc_root_reference_t* tmpRef = NULL;
    foxgc_soft_reference_t* ref = foxgc_api_object_get_data(cachedStringEntry.data.userdata->userGarbageCollectableData);
    foxgc_object_t* entryObj = foxgc_api_reference_get(ref, fluffyvm_get_root(vm), &tmpRef);
    if (!entryObj)
      goto cache_miss;
    struct cache_entry* entry = foxgc_api_object_get_data(entryObj);
    foxgc_api_root_add(vm->heap, value_get_object_ptr(entry->string), fluffyvm_get_root(vm), rootRef);
    foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmpRef);
    return entry->string;
  }

  cache_miss:
  ref_counter_inc(this->additionalData); 
  struct string_cache_additional_data* additionalData = this->additionalData->data;
  
  struct value newString = this->allocator(vm, string, len, rootRef, this->udata, Block_copy(^void () {
    pthread_cond_broadcast(&additionalData->onEntryInvalidated);
    ref_counter_dec(this->additionalData); 
  }));
  if (newString.type == FLUFFYVM_TVALUE_NOT_PRESENT) {
    ref_counter_dec(this->additionalData); 
    return value_not_present();
  }
  
  foxgc_root_reference_t* tmp = NULL;
  foxgc_object_t* cacheEntry = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), &tmp, vm->stringCacheStaticData->desc_string_cache_entry, NULL);
  if (!cacheEntry)
    goto try_later;
  struct cache_entry* entry = foxgc_api_object_get_data(cacheEntry);
  value_copy(&entry->string, newString);
  foxgc_api_write_field(cacheEntry, 0, cacheEntry);
  foxgc_api_write_field(cacheEntry, 1, value_get_object_ptr(newString));

  foxgc_root_reference_t* tmp2 = NULL;
  foxgc_soft_reference_t* reference = foxgc_api_new_soft_reference(vm->heap, fluffyvm_get_root(vm), &tmp2, entry->gc_this);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmp);
  
  if (reference == NULL) 
    goto try_later;
  
  foxgc_root_reference_t* tmp3 = NULL; 
  struct value entryValue = value_new_garbage_collectable_userdata(vm, vm->modules.stringCache.moduleID, vm->modules.stringCache.type.softReference, foxgc_api_reference_get_reference_object(reference), &tmp3);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmp2);
  
  if (entryValue.type == FLUFFYVM_TVALUE_NOT_PRESENT)
    goto try_later;
  
  // Its fine if this failed to set
  // we can try add it to cache later
  pthread_rwlock_wrlock(&this->rwlock);
  addCacheItem(vm, string, len, entryValue);
  pthread_rwlock_unlock(&this->rwlock);

  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmp3);
  
  try_later:
  return newString;
  */
}

void string_cache_poll(struct fluffyvm* vm, struct string_cache* cache, struct timespec* timeout) {
  nanosleep(timeout, NULL);
  
  /*
  struct string_cache_additional_data* additionalData = cache->additionalData->data;
  pthread_mutex_lock(&additionalData->onEntryInvalidatedLock);
  if (timeout && pthread_cond_timedwait(&additionalData->onEntryInvalidated, &additionalData->onEntryInvalidatedLock, timeout) != 0) {
    pthread_mutex_unlock(&additionalData->onEntryInvalidatedLock);
    return;
  }

  puts("Clearing cache");

  pthread_rwlock_wrlock(&cache->rwlock);
    
  pthread_rwlock_unlock(&cache->rwlock);
  pthread_mutex_unlock(&additionalData->onEntryInvalidatedLock);
  */
}





