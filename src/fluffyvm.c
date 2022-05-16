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
#include "coroutine.h"
#include "closure.h"
#include "stack.h"

// X macro idea is awsome
#define STATIC_STRINGS \
  X(outOfMemory, "out of memory") \
  X(outOfMemoryWhileHandlingError, "out of memory while handling another error") \
  X(outOfMemoryWhileAnErrorOccured, "out of memory while another error occured") \
  X(strtodDidNotProcessAllTheData, "strtod did not process all the data") \
  X(typenames.nil, "nil") \
  X(typenames.string, "string") \
  X(typenames.doubleNum, "double") \
  X(typenames.longNum, "long") \
  X(typenames.table, "table") \
  X(typenames.closure, "function") \
  X(invalidCapacity, "invalid capacity") \
  X(badKey, "bad key") \
  X(protobufFailedToUnpackData, "protobuf failed to unpack data") \
  X(unsupportedBytecode, "unsupported bytecode version") \
  X(pthreadCreateError, "pthread_create call unsuccessful") \
  X(invalidBytecode, "invalid bytecode") \
  X(cannotResumeDeadCoroutine, "cannot resume dead corotine") \
  X(invalidArrayBound, "invalid array bound") \
  X(cannotResumeRunningCoroutine, "cannot resume running coroutine") \
  X(cannotSuspendTopLevelCoroutine, "cannot suspend top level coroutine") \
  X(illegalInstruction, "illegal instruction") \
  X(stackOverflow, "stack overflow") \
  X(stackUnderflow, "stack underflow") \
  X(attemptToIndexNonIndexableValue, "attempt to index not indexable value") \
  X(attemptToCallNonCallableValue, "attempt to call not callable value")

#define COMPONENTS \
  X(value) \
  X(statics) \
  X(stack) \
  X(hashtable) \
  X(bytecode) \
  X(bytecode_loader_json) \
  X(closure) \
  X(coroutine)

// Initialize static stuffs
static bool statics_init(struct fluffyvm* this) {
  memset(&this->staticStrings, 0, sizeof(this->staticStrings));
  
# define X(name, string, ...) { \
    foxgc_root_reference_t* tmpRef = NULL;\
    struct value tmp = value_new_string(this, (string), &tmpRef); \
    if (tmp.type == FLUFFYVM_TVALUE_NOT_PRESENT) \
      return false; \
    value_copy(&this->staticStrings.name, &tmp); \
    foxgc_api_root_add(this->heap, value_get_object_ptr(tmp), this->staticDataRoot, &this->staticStrings.name ## RootRef); \
    foxgc_api_remove_from_root2(this->heap, fluffyvm_get_root(this), tmpRef); \
  }
  STATIC_STRINGS
# undef X

  return true;
}

static void statics_cleanup(struct fluffyvm* this) {
# define X(name, string, ...) \
  if (this->staticStrings.name ## RootRef) \
    foxgc_api_remove_from_root2(this->heap, this->staticDataRoot, this->staticStrings.name ## RootRef);
  STATIC_STRINGS
# undef X
}

// Make current thread managed
static void initThread(struct fluffyvm* this, int* threadIdStorage) {
  this->numberOfManagedThreads++;

  foxgc_root_t* root = foxgc_api_new_root(this->heap); 
  pthread_setspecific(this->currentThreadRootKey, root);
  pthread_setspecific(this->errMsgKey, NULL);
  pthread_setspecific(this->errMsgRootRefKey, NULL);
  pthread_setspecific(this->currentCoroutine, NULL);
  pthread_setspecific(this->currentCoroutineRootRef, NULL);

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

typedef void (*cleanup_call)(struct fluffyvm* vm);

// Zero initCount mean nothing initialized
// -1 initCount mean all initialized
static void commonCleanup(struct fluffyvm* this, int initCounts) {
  // Using switch passthrough
  // to correctly call clean up
  // for all initialized stuffs
  // and in correct order
  
  cleanup_call calls[] = {
# define X(name, ...) name ## _cleanup,
  COMPONENTS
# undef X
  };

  if (initCounts < 0)
    initCounts = sizeof(calls) / sizeof(calls[0]);

  // Cast to smaller integer in this case
  // is safe because why would i have over
  // 2 billions components to clean up
  for (int i = initCounts - 1; i >= 0; i--)
    calls[i](this);

  foxgc_api_delete_root(this->heap, this->staticDataRoot);

  cleanThread(this);
  pthread_key_delete(this->currentThreadRootKey);
  pthread_key_delete(this->errMsgKey);
  pthread_key_delete(this->errMsgRootRefKey);
  pthread_key_delete(this->currentThreadID);
  pthread_key_delete(this->currentCoroutine);
  pthread_key_delete(this->currentCoroutineRootRef);
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
  pthread_key_create(&this->currentCoroutine, NULL);
  pthread_key_create(&this->currentCoroutineRootRef, NULL);
  
  int* tidStorage = malloc(sizeof(int));
  initThread(this, tidStorage);
  
  this->staticDataRoot = foxgc_api_new_root(heap);

  int initCounts = 0;

  // Start initializing stuffs
# define X(name, ...) \
  initCounts++; \
  if (!name ## _init(this)) \
    goto error;
  COMPONENTS
# undef X

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

struct fluffyvm_coroutine* fluffyvm_get_executing_coroutine(struct fluffyvm* this) {
  validateThisThread(this);
  return pthread_getspecific(this->currentCoroutine);
}

void fluffyvm_set_executing_coroutine(struct fluffyvm* this, struct fluffyvm_coroutine* co) {
  validateThisThread(this);
  foxgc_root_reference_t* tmp;
  if ((tmp = pthread_getspecific(this->currentCoroutineRootRef)))
    foxgc_api_remove_from_root2(this->heap, fluffyvm_get_root(this), tmp);

  if (co) {
    foxgc_api_root_add(this->heap, co->gc_this, fluffyvm_get_root(this), &tmp);
    pthread_setspecific(this->currentCoroutineRootRef, tmp);
  }
  pthread_setspecific(this->currentCoroutine, co);
}



