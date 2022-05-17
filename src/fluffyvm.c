#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "config.h"
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
  X(attemptToCallNonCallableValue, "attempt to call not callable value") \
  X(notInCoroutine, "not in coroutine") \
  X(coroutineNestTooDeep, "coroutine nest too deep")

#define COMPONENTS \
  X(stack) \
  X(value) \
  X(statics) \
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
static void initThread(struct fluffyvm* this, int* threadIdStorage, foxgc_root_t* newRoot, struct fluffyvm_stack* coroutineStack) {
  this->numberOfManagedThreads++;

  pthread_setspecific(this->currentThreadRootKey, newRoot);
  pthread_setspecific(this->errMsgKey, NULL);
  pthread_setspecific(this->errMsgRootRefKey, NULL);
  pthread_setspecific(this->coroutinesStack, coroutineStack);

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

  cleanThread(this);
  foxgc_api_delete_root(this->heap, this->staticDataRoot);

  pthread_key_delete(this->currentThreadRootKey);
  pthread_key_delete(this->errMsgKey);
  pthread_key_delete(this->errMsgRootRefKey);
  pthread_key_delete(this->currentThreadID);
  pthread_key_delete(this->coroutinesStack);
  free(this);
}

struct fluffyvm* fluffyvm_new(struct foxgc_heap* heap) {
  struct fluffyvm* this = malloc(sizeof(*this));
  if (this == NULL)
    return this;
  
  this->heap = heap;
  this->numberOfManagedThreads = 0;
  this->currentAvailableThreadID = 0;
  this->hasInit = false;

  pthread_key_create(&this->currentThreadRootKey, NULL);
  pthread_key_create(&this->errMsgKey, NULL);
  pthread_key_create(&this->errMsgRootRefKey, NULL);
  pthread_key_create(&this->currentThreadID, NULL);
  pthread_key_create(&this->coroutinesStack, NULL);
  
  int* tidStorage = malloc(sizeof(int));
  int initCounts = 0;
 
  // Stack component needed to bootstrap
  // other components (its kinda chicken
  // or egg problem)
  this->stackStaticData = NULL;
  stack_init(this);

  // Bootstrap current thread to be managed
  foxgc_root_t* newRoot = foxgc_api_new_root(this->heap);
  if (!newRoot)
    goto error;
  
  pthread_setspecific(this->currentThreadRootKey, newRoot);

  foxgc_root_reference_t* rootRef = NULL;
  struct fluffyvm_stack* coroutinesStack = stack_new(this, &rootRef, FLUFFYVM_MAX_COROUTINE_NEST);
  if (!coroutinesStack)
    goto error; 
  
  initThread(this, tidStorage, newRoot, coroutinesStack);
  this->hasInit = true;
  // Done bootstrapping

  this->staticDataRoot = foxgc_api_new_root(heap);

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
  if (!vm->hasInit)
    return false;

  validateThisThread(vm);
  return pthread_getspecific(vm->errMsgKey) != NULL;
}

struct value fluffyvm_get_errmsg(struct fluffyvm* vm) {
  if (!vm->hasInit)
    return value_not_present();
  
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
  foxgc_root_t* newRoot;
  struct fluffyvm_stack* coroutinesStack;
};

static void* threadStub(void* _args) {
  struct thread_args* args = _args;
  fluffyvm_thread_routine_t routine = args->routine;
  void* routine_args = args;
  
  initThread(args->vm, args->tidStorage, args->newRoot, args->coroutinesStack);

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
    goto cannot_alloc_data;
  }

  data->routine = routine;
  data->vm = this;
  data->args = args;

  int* storage = malloc(sizeof(int));
  data->tidStorage = storage;
  if (!data->tidStorage) {
    fluffyvm_set_errmsg(this, this->staticStrings.outOfMemory);
    goto cannot_alloc_tid_storage;
  }

  foxgc_root_t* newRoot = foxgc_api_new_root(this->heap);
  if (!newRoot) {
    fluffyvm_set_errmsg(this, this->staticStrings.outOfMemory);
    goto cannot_alloc_root;
  }

  data->newRoot = newRoot;

  foxgc_root_reference_t* rootRef = NULL;
  data->coroutinesStack = stack_new(this, &rootRef, FLUFFYVM_MAX_COROUTINE_NEST);
  if (!data->coroutinesStack) {
    fluffyvm_set_errmsg(this, this->staticStrings.outOfMemory);
    goto cannot_alloc_coroutine_stack;
  } 
  foxgc_root_reference_t* rootRef2 = NULL;
  foxgc_api_root_add(this->heap, data->coroutinesStack->gc_this, newRoot, &rootRef2);
  foxgc_api_remove_from_root2(this->heap, fluffyvm_get_root(this), rootRef);

  if (pthread_create(newthread, attr, threadStub, data) != 0) {
    fluffyvm_set_errmsg(this, this->staticStrings.pthreadCreateError);
    goto cannot_create_thread;
  }

  return true;
  cannot_create_thread:
  cannot_alloc_coroutine_stack:
  foxgc_api_delete_root(this->heap, newRoot);
  cannot_alloc_root:
  free(data->tidStorage);
  cannot_alloc_tid_storage:
  free(data);
  cannot_alloc_data:
  return false;
}

foxgc_root_t* fluffyvm_get_root(struct fluffyvm* this) {
  validateThisThread(this);
  return pthread_getspecific(this->currentThreadRootKey);
}

struct fluffyvm_coroutine* fluffyvm_get_executing_coroutine(struct fluffyvm* this) {
  validateThisThread(this);
  void* co = NULL;
  if (!stack_peek(this, pthread_getspecific(this->coroutinesStack), &co))
    return NULL;
  return co;
}

void fluffyvm_pop_current_coroutine(struct fluffyvm* this) {
  validateThisThread(this);
  bool res = stack_pop(this, pthread_getspecific(this->coroutinesStack), NULL, NULL);
  assert(res);
}

bool fluffyvm_push_current_coroutine(struct fluffyvm* this, struct fluffyvm_coroutine* co) {
  validateThisThread(this);
  return stack_push(this, pthread_getspecific(this->coroutinesStack), co->gc_this);
}


