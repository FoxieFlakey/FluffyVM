#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "fluffyvm.h"
#include "foxgc.h"
#include "hashtable.h"
#include "value.h"
#include "fluffyvm_types.h"
#include "bytecode.h"

// Make current thread managed
static void initThread(struct fluffyvm* this) {
  this->numberOfManagedThreads++;

  foxgc_root_t* root = foxgc_api_new_root(this->heap); 
  pthread_setspecific(this->currentThreadRootKey, root);
  pthread_setspecific(this->errMsgKey, NULL);
  pthread_setspecific(this->errMsgRootRefKey, NULL);
}

static void validateThisThread(struct fluffyvm* this) {
  // If thread root not set, abort
  if (pthread_getspecific(this->currentThreadRootKey) == NULL) {
    abort();
  }
}

// Cleanup resources allocated for current thread
static void cleanThread(struct fluffyvm* this) {
  foxgc_root_reference_t* ref = pthread_getspecific(this->errMsgRootRefKey);
  if (ref)
    foxgc_api_remove_from_root2(this->heap, fluffyvm_get_root(this), ref);
  free(pthread_getspecific(this->errMsgRootRefKey));
  
  foxgc_api_delete_root(this->heap, pthread_getspecific(this->currentThreadRootKey));
  
  this->numberOfManagedThreads--;
}

static void commonCleanup(struct fluffyvm* this) {
  foxgc_api_delete_root(this->heap, this->staticDataRoot);

  cleanThread(this);
  pthread_key_delete(this->currentThreadRootKey);
  pthread_key_delete(this->errMsgKey);
  pthread_key_delete(this->errMsgRootRefKey);
  free(this);
}

struct fluffyvm* fluffyvm_new(struct foxgc_heap* heap) {
  struct fluffyvm* this = malloc(sizeof(*this));
  this->heap = heap;
  this->numberOfManagedThreads = 0;
  pthread_key_create(&this->currentThreadRootKey, NULL);
  pthread_key_create(&this->errMsgKey, NULL);
  pthread_key_create(&this->errMsgRootRefKey, NULL);
  initThread(this);
  
  this->staticDataRoot = foxgc_api_new_root(heap);

  int initCounts = 0;

  // Start initializing stuffs
  initCounts++;
  if (!value_init(this))
    goto error;
  
  initCounts++;
  if (!hashtable_init(this))
    goto error;

  initCounts++;
  if (!bytecode_init(this))
    goto error;

  return this;
  
  error:
  // Using switch passthrough
  // to correctly call clean up
  // for all initialized stuffs
  // and in correct order
  switch (initCounts) {
    case 3:
      bytecode_cleanup(this);
    case 2:
      hashtable_cleanup(this);
    case 1:
      value_cleanup(this);
  }
  commonCleanup(this);
  return NULL;
}

void fluffyvm_set_errmsg(struct fluffyvm* vm, struct value val) {
  validateThisThread(vm);
  struct value* msg;
  if ((msg = pthread_getspecific(vm->errMsgKey))) {
    foxgc_root_reference_t* ref;
    if ((ref = pthread_getspecific(vm->errMsgRootRefKey)))
      foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), ref);
    
    free(msg);
  }

  if (val.type == FLUFFYVM_TVALUE_NOT_PRESENT) {
    pthread_setspecific(vm->errMsgKey, NULL);
    pthread_setspecific(vm->errMsgRootRefKey, NULL);
    return;
  }

  foxgc_object_t* ptr;
  if ((ptr = value_get_object_ptr(val))) {
    foxgc_root_reference_t* ref = NULL;
    foxgc_api_root_add(vm->heap, ptr, fluffyvm_get_root(vm), &ref);
  } else {
    pthread_setspecific(vm->errMsgRootRefKey, NULL);
  }

  msg = malloc(sizeof(val));
  value_copy(msg, &val);
  pthread_setspecific(vm->errMsgKey, msg);
}

bool fluffyvm_is_errmsg_present(struct fluffyvm* vm) {
  validateThisThread(vm);
  return pthread_getspecific(vm->errMsgKey) != NULL;
}

struct value fluffyvm_get_errmsg(struct fluffyvm* vm) {
  validateThisThread(vm);
  if (!fluffyvm_is_errmsg_present(vm))
    return value_not_present();
  return *((struct value*) pthread_getspecific(vm->errMsgKey));
}

void fluffyvm_free(struct fluffyvm* this) {
  validateThisThread(this);

  // There still other thread running
  // 1 of the threads is the caller thread
  if (this->numberOfManagedThreads > 1) {
    abort();
  }

  bytecode_cleanup(this);
  hashtable_cleanup(this);
  value_cleanup(this);
  
  commonCleanup(this);
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
  validateThisThread(this);
  
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

