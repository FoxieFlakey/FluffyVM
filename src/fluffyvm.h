#ifndef header_1650783109_fluffyvm_h
#define header_1650783109_fluffyvm_h

#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>

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
};

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





