#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "fluffyvm.h"
#include "foxgc.h"
#include "hashtable.h"
#include "value.h"
#include "fluffyvm_types.h"

// Make current thread managed
static void initThread(struct fluffyvm* this) {
  this->numberOfManagedThreads++;

  foxgc_root_t* root = foxgc_api_new_root(this->heap); 
  pthread_setspecific(this->currentThreadRootKey, root);
}

static void validateThisThread(struct fluffyvm* this) {
  // If thread root not set, abort
  if (pthread_getspecific(this->currentThreadRootKey) == NULL) {
    abort();
  }
}

// Cleanup resources allocated for current thread
static void cleanThread(struct fluffyvm* this) {
  foxgc_api_delete_root(this->heap, pthread_getspecific(this->currentThreadRootKey));
  
  this->numberOfManagedThreads--;
}

struct fluffyvm* fluffyvm_new(struct foxgc_heap* heap) {
  struct fluffyvm* this = malloc(sizeof(*this));
  this->heap = heap;
  this->numberOfManagedThreads = 0;
  pthread_key_create(&this->currentThreadRootKey, NULL);
  initThread(this);
  
  // Start initializing stuffs
  value_init(this);
  hashtable_init(this);

  return this;
}

void fluffyvm_free(struct fluffyvm* this) {
  validateThisThread(this);

  // There still other thread running
  // 1 of the threads is the caller thread
  if (this->numberOfManagedThreads > 1) {
    abort();
  }

  hashtable_cleanup(this);
  value_cleanup(this);

  cleanThread(this);
  pthread_key_delete(this->currentThreadRootKey);
  free(this);
}

struct thread_args {
  struct fluffyvm* vm;
  fluffyvm_thread_routine_t routine;
  void* args;
};

static void* threadStub(void* _args) {
  struct thread_args* args = _args;
  fluffyvm_thread_routine_t routine = args->routine;
  void* routine_args = args;
  
  initThread(args->vm);

  free(args);
  void* ret = routine(routine_args);
  
  // I know there is destructor for pthread key
  // and its called at thread exit but for now
  // i use this 
  cleanThread(args->vm);
  return ret;
}

bool fluffyvm_start_thread(struct fluffyvm* this, pthread_t* newthread, pthread_attr_t* attr, fluffyvm_thread_routine_t routine, void* args) {
  struct thread_args* data = malloc(sizeof(*data));
  data->routine = routine;
  data->vm = this;
  data->args = args;

  if (!pthread_create(newthread, attr, threadStub, data)) {
    free(data);
    return false;
  }

  return true;
}

foxgc_root_t* fluffyvm_get_root(struct fluffyvm* this) {
  validateThisThread(this);
  return pthread_getspecific(this->currentThreadRootKey);
}

