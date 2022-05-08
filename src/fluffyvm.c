#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fluffyvm.h"
#include "foxgc.h"
#include "hashtable.h"
#include "loader/bytecode/json.h"
#include "value.h"
#include "fluffyvm_types.h"
#include "bytecode.h"

#define new_static_string(vm, name, string) do { \
  foxgc_root_reference_t* tmpRef;\
  struct value tmp = value_new_string(vm, (string), &tmpRef); \
  if (tmp.type == FLUFFYVM_TVALUE_NOT_PRESENT) \
    return false; \
  value_copy(&vm->staticStrings.name, &tmp); \
  foxgc_api_root_add(vm->heap, value_get_object_ptr(tmp), vm->staticDataRoot, &vm->staticStrings.name ## RootRef); \
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmpRef); \
} while (0)

#define clean_static_string(vm, name) do { \
  if (vm->staticStrings.name ## RootRef) \
    foxgc_api_remove_from_root2(vm->heap, vm->staticDataRoot, vm->staticStrings.name ## RootRef); \
} while (0)

// Initialize static strings
static bool initStatic(struct fluffyvm* this) {
  memset(&this->staticStrings, 0, sizeof(this->staticStrings));
  
  new_static_string(this, outOfMemory, "out of memory");
  new_static_string(this, outOfMemoryWhileHandlingError, "out of memory while handling another error");
  new_static_string(this, outOfMemoryWhileAnErrorOccured, "out of memory while another error occured");
  new_static_string(this, strtodDidNotProcessAllTheData, "strtod did not process all the data");
  new_static_string(this, typenames.nil, "nil");
  new_static_string(this, typenames.string, "string");
  new_static_string(this, typenames.doubleNum, "double");
  new_static_string(this, typenames.longNum, "long");
  new_static_string(this, typenames.table, "table");
  new_static_string(this, invalidCapacity, "invalid capacity");
  new_static_string(this, badKey, "bad key"); 
  new_static_string(this, protobufFailedToUnpackData, "protobuf failed to unpack data");
  new_static_string(this, unsupportedBytecode, "unsupported bytecode version");
  new_static_string(this, pthreadCreateError, "pthread_create call unsuccessful");
  new_static_string(this, invalidBytecode, "invalid bytecode");
  new_static_string(this, invalidArrayBound, "invalid array bound");

  return true;
}

static void cleanStatic(struct fluffyvm* this) {
  clean_static_string(this, outOfMemory);
  clean_static_string(this, outOfMemoryWhileAnErrorOccured);
  clean_static_string(this, outOfMemoryWhileHandlingError);
  clean_static_string(this, strtodDidNotProcessAllTheData);
  clean_static_string(this, typenames.nil);
  clean_static_string(this, typenames.string);
  clean_static_string(this, typenames.longNum);
  clean_static_string(this, typenames.doubleNum);
  clean_static_string(this, typenames.table);
  clean_static_string(this, badKey);
  clean_static_string(this, invalidCapacity);
  clean_static_string(this, unsupportedBytecode);
  clean_static_string(this, pthreadCreateError);
  clean_static_string(this, invalidBytecode);
  clean_static_string(this, invalidArrayBound);
}

// Make current thread managed
static void initThread(struct fluffyvm* this, int* threadIdStorage) {
  this->numberOfManagedThreads++;

  foxgc_root_t* root = foxgc_api_new_root(this->heap); 
  pthread_setspecific(this->currentThreadRootKey, root);
  pthread_setspecific(this->errMsgKey, NULL);
  pthread_setspecific(this->errMsgRootRefKey, NULL);

  int id = atomic_fetch_add(&this->currentAvailableThreadID, 1);
  *threadIdStorage = id;

  pthread_setspecific(this->currentThreadID, threadIdStorage);
}

static void validateThisThread(struct fluffyvm* this) {
  // If thread root not set, abort
  if (pthread_getspecific(this->currentThreadRootKey) == NULL) {
    abort();
  }
}

int fluffyvm_get_thread_id(struct fluffyvm* this) {
  validateThisThread(this);
  return *((int*) pthread_getspecific(this->currentThreadID));
}

// Cleanup resources allocated for current thread
static void cleanThread(struct fluffyvm* this) {
  foxgc_root_reference_t* ref = pthread_getspecific(this->errMsgRootRefKey);
  if (ref)
    foxgc_api_remove_from_root2(this->heap, fluffyvm_get_root(this), ref);
  free(pthread_getspecific(this->errMsgKey));
  free(pthread_getspecific(this->currentThreadID));

  foxgc_api_delete_root(this->heap, pthread_getspecific(this->currentThreadRootKey));
  
  this->numberOfManagedThreads--;
}

static void commonCleanup(struct fluffyvm* this, int initCounts) {
  // Using switch passthrough
  // to correctly call clean up
  // for all initialized stuffs
  // and in correct order
  switch (initCounts) {
    case -1:
    case 5:
      bytecode_loader_json_cleanup(this);
    case 4:
      bytecode_cleanup(this);
    case 3:
      hashtable_cleanup(this);
    case 2:
      cleanStatic(this);
    case 1:
      value_cleanup(this);
  }
  foxgc_api_delete_root(this->heap, this->staticDataRoot);

  cleanThread(this);
  pthread_key_delete(this->currentThreadRootKey);
  pthread_key_delete(this->errMsgKey);
  pthread_key_delete(this->errMsgRootRefKey);
  pthread_key_delete(this->currentThreadID);
  free(this);
}

struct fluffyvm* fluffyvm_new(struct foxgc_heap* heap) {
  struct fluffyvm* this = malloc(sizeof(*this));
  if (this == NULL)
    return this;
  
  this->heap = heap;
  this->numberOfManagedThreads = 0;
  this->currentAvailableThreadID = 0;
  pthread_key_create(&this->currentThreadRootKey, NULL);
  pthread_key_create(&this->errMsgKey, NULL);
  pthread_key_create(&this->errMsgRootRefKey, NULL);
  pthread_key_create(&this->currentThreadID, NULL);
  
  int* tidStorage = malloc(sizeof(int));
  initThread(this, tidStorage);
  
  this->staticDataRoot = foxgc_api_new_root(heap);

  int initCounts = 0;

  // Start initializing stuffs
  initCounts++;
  if (!value_init(this))
    goto error;
  
  initCounts++;
  if (!initStatic(this))
    goto error;

  initCounts++;
  if (!hashtable_init(this))
    goto error;

  initCounts++;
  if (!bytecode_init(this))
    goto error;

  initCounts++;
  if (!bytecode_loader_json_init(this))
    goto error;

  return this;
  
  error: 
  commonCleanup(this, initCounts);
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
  if (this->numberOfManagedThreads > 1)
    abort();
  
  commonCleanup(this, -1);
}

struct thread_args {
  struct fluffyvm* vm;
  fluffyvm_thread_routine_t routine;
  void* args;
  int* tidStorage;
};

static void* threadStub(void* _args) {
  struct thread_args* args = _args;
  fluffyvm_thread_routine_t routine = args->routine;
  void* routine_args = args;
  
  initThread(args->vm, args->tidStorage);

  void* ret = routine(routine_args);
  
  // I know there is destructor for pthread key
  // and its called at thread exit but for now
  // i use this 
  cleanThread(args->vm);
  free(args);
  return ret;
}

bool fluffyvm_start_thread(struct fluffyvm* this, pthread_t* newthread, pthread_attr_t* attr, fluffyvm_thread_routine_t routine, void* args) {
  validateThisThread(this);
  
  struct thread_args* data = malloc(sizeof(*data));
  if (!data) {
    fluffyvm_set_errmsg(this, this->staticStrings.outOfMemory);
    return false;
  }

  data->routine = routine;
  data->vm = this;
  data->args = args;

  int* storage = malloc(sizeof(int));
  data->tidStorage = storage;

  if (!data->tidStorage) {
    fluffyvm_set_errmsg(this, this->staticStrings.outOfMemory);
    free(data);
    return false;
  }

  if (pthread_create(newthread, attr, threadStub, data) != 0) {
    fluffyvm_set_errmsg(this, this->staticStrings.pthreadCreateError);
    free(data->tidStorage);
    free(data);
    return false;
  }

  return true;
}

foxgc_root_t* fluffyvm_get_root(struct fluffyvm* this) {
  validateThisThread(this);
  return pthread_getspecific(this->currentThreadRootKey);
}

