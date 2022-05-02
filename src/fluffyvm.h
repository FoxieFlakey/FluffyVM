#ifndef header_1650783109_fluffyvm_h
#define header_1650783109_fluffyvm_h

#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>

#include "value.h"
#include "foxgc.h"
#include "collections/list.h"

struct fluffyvm {
  foxgc_heap_t* heap;
  
  foxgc_root_t* root;

  // Essentially scratch pad
  // For each thread to avoid
  // lock contention on main root
  // foxgc_root_t*
  pthread_key_t currentThreadRootKey;
  
  atomic_int numberOfManagedThreads;
  
  struct value_static_data* valueStaticData;
  struct hashtable_static_data* hashTableStaticData;
  struct bytecode_static_data* bytecodeStaticData;

  foxgc_root_t* staticDataRoot;

  // Essentially like errno
  pthread_key_t errMsgKey;
  pthread_key_t errMsgRootRefKey;
};

// errmsg in all docs refer to these two functions
// unless noted
void fluffyvm_set_errmsg(struct fluffyvm* vm, struct value val);
struct value fluffyvm_get_errmsg(struct fluffyvm* vm);
bool fluffyvm_is_errmsg_present(struct fluffyvm* vm);
static inline void fluffyvm_clear_errmsg(struct fluffyvm* vm) {
  fluffyvm_set_errmsg(vm, value_not_present());
}

typedef void* (^fluffyvm_thread_routine_t)(void*);

// Create new VM and set current thread
// as managed thread
struct fluffyvm* fluffyvm_new(struct foxgc_heap* heap);

bool fluffyvm_is_managed(struct fluffyvm* this);

//////////////////////////////////////////////////
// API calls below this line will call abort    //
// if the caller thread is not managed thread   //
//////////////////////////////////////////////////

// Use this to start thread if you
// want new thread capable of using
// VM API calls
bool fluffyvm_start_thread(struct fluffyvm* this, pthread_t* newthread, pthread_attr_t* attr, fluffyvm_thread_routine_t routine, void* args);
foxgc_root_t* fluffyvm_get_root(struct fluffyvm* this);

// WARNING: Make sure at the moment you call
// this there are no access to this in the future
void fluffyvm_free(struct fluffyvm* this);

#endif





