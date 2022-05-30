#define FLUFFYVM_INTERNAL

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "fiber.h"
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
#include "api_layer/lua54.h"

#define COMPONENTS \
  X(stack) \
  X(value) \
  X(statics) \
  X(hashtable) \
  X(bytecode) \
  X(bytecode_loader_json) \
  X(closure) \
  X(coroutine) \
  X(fluffyvm_compat_layer_lua54)

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
  FLUFFYVM_STATIC_STRINGS
# undef X

  return true;
}

static void statics_cleanup(struct fluffyvm* this) {
# define X(name, string, ...) \
  if (this->staticStrings.name ## RootRef) \
    foxgc_api_remove_from_root2(this->heap, this->staticDataRoot, this->staticStrings.name ## RootRef);
  FLUFFYVM_STATIC_STRINGS
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

static bool lateInit(struct fluffyvm* this) {
  // Create coroutine for main thread
  foxgc_root_reference_t* closureRootRef = NULL;
  foxgc_root_reference_t* coroutineRootRef = NULL;
  
  struct fluffyvm_closure* dummyClosure = closure_from_cfunction(this, &closureRootRef, NULL, NULL, NULL, fluffyvm_get_global(this));
  if (!dummyClosure)
    goto error;
  
  struct fluffyvm_coroutine* mainThread = coroutine_new(this, &coroutineRootRef, dummyClosure);
  if (!mainThread) {
    foxgc_api_remove_from_root2(this->heap, fluffyvm_get_root(this), closureRootRef);
    goto error;
  }
  
  fluffyvm_push_current_coroutine(this, mainThread);

  foxgc_api_remove_from_root2(this->heap, fluffyvm_get_root(this), closureRootRef);
  foxgc_api_remove_from_root2(this->heap, fluffyvm_get_root(this), coroutineRootRef);

  mainThread->fiber->state = FIBER_RUNNING;
  mainThread->isNativeThread = true;
  
  return true;
  
  error:
  return false;
}

static void validateThisThread(struct fluffyvm* this) {
  // If thread root not set, abort
  if (!pthread_getspecific(this->currentThreadRootKey))
    abort();
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

  for (int i = initCounts - 1; i >= 0; i--)
    calls[i](this);

  cleanThread(this);
  foxgc_api_delete_root(this->heap, this->staticDataRoot);

  pthread_key_delete(this->currentThreadRootKey);
  pthread_key_delete(this->errMsgKey);
  pthread_key_delete(this->errMsgRootRefKey);
  pthread_key_delete(this->currentThreadID);
  pthread_key_delete(this->coroutinesStack);
  pthread_rwlock_destroy(&this->globalTableLock);
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
  pthread_rwlock_init(&this->globalTableLock, NULL);
  
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

  this->modules.compatLayer_Lua54.moduleID = value_get_module_id();
  this->modules.compatLayer_Lua54.type.userdata = 1;
  this->globalTableRootRef = NULL;

  // Create global table
  foxgc_root_reference_t* globalTableRootRef = NULL;
  struct value globalTable = value_new_table(this, FLUFFYVM_HASHTABLE_DEFAULT_LOAD_FACTOR, 16, &globalTableRootRef);
  
  if (globalTable.type == FLUFFYVM_TVALUE_NOT_PRESENT)
    goto error;
  fluffyvm_set_global(this, globalTable);
  foxgc_api_remove_from_root2(this->heap, fluffyvm_get_root(this), globalTableRootRef);
  
  lateInit(this);    

  return this;
  
  error: 
  commonCleanup(this, initCounts);
  return NULL;
}

void fluffyvm_set_global(struct fluffyvm* this, struct value val) {
  if (val.type == FLUFFYVM_TVALUE_NOT_PRESENT)
    abort();

  pthread_rwlock_wrlock(&this->globalTableLock);

  if (this->globalTableRootRef)
    foxgc_api_remove_from_root2(this->heap, this->staticDataRoot, this->globalTableRootRef);
  value_copy(&this->globalTable, &val);
  
  foxgc_object_t* obj = value_get_object_ptr(val);
  if (obj)
    foxgc_api_root_add(this->heap, obj, this->staticDataRoot, &this->globalTableRootRef);
  else
    this->globalTableRootRef = NULL;

  pthread_rwlock_unlock(&this->globalTableLock);
}

struct value fluffyvm_get_global(struct fluffyvm* this) {
  pthread_rwlock_rdlock(&this->globalTableLock);
  
  if (this->globalTable.type == FLUFFYVM_TVALUE_NOT_PRESENT)
    abort();
  struct value val = this->globalTable;
  
  pthread_rwlock_unlock(&this->globalTableLock);
  return val;
}

void fluffyvm_set_errmsg(struct fluffyvm* vm, struct value val) {
  validateThisThread(vm);
  struct value* msg;
  if ((msg = pthread_getspecific(vm->errMsgKey))) {
    foxgc_root_reference_t* ref;
    if ((ref = pthread_getspecific(vm->errMsgRootRefKey)))
      foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), ref);
    free(msg);
    pthread_setspecific(vm->errMsgRootRefKey, NULL);
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
    pthread_setspecific(vm->errMsgRootRefKey, ref);
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
  
  pthread_cond_t* initCompletionSignal;
  volatile bool status;
  volatile struct value errorMessage;
};

static void* threadStub(void* _args) {
  struct thread_args* args = _args;
  fluffyvm_thread_routine_t routine = args->routine;
  void* routine_args = args;
  void* ret = NULL;

  // This call cannot be moved to the
  // fluffyvm_start_thread due its
  // setting something on pthread keys  
  initThread(args->vm, args->tidStorage, args->newRoot, args->coroutinesStack);
  
  args->status = lateInit(args->vm);
  struct value errMsg = fluffyvm_get_errmsg(args->vm);
  value_copy((struct value*) &args->errorMessage, &errMsg);
  pthread_cond_signal(args->initCompletionSignal);
  
  if (!args->status)
    goto terminate_thread;

  ret = routine(routine_args);
  
  // I know there is destructor for pthread key
  // and its called at thread exit but for now
  // i use this 
  terminate_thread:
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

  pthread_mutex_t dummyLock = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t initCompletionSignal = PTHREAD_COND_INITIALIZER;
  data->status = true;
  data->initCompletionSignal = &initCompletionSignal;

  if (pthread_create(newthread, attr, threadStub, data) != 0) {
    fluffyvm_set_errmsg(this, this->staticStrings.pthreadCreateError);
    goto cannot_create_thread;
  }

  pthread_mutex_lock(&dummyLock);
  pthread_cond_wait(&initCompletionSignal, &dummyLock);
  pthread_mutex_unlock(&dummyLock);

  if (!data->status)
    goto thread_init_failed;

  pthread_mutex_destroy(&dummyLock);
  pthread_cond_destroy(&initCompletionSignal);
  return true;

  thread_init_failed:
  fluffyvm_set_errmsg(this, data->errorMessage);
  cannot_create_thread:
  pthread_mutex_destroy(&dummyLock);
  pthread_cond_destroy(&initCompletionSignal);
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


