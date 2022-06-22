#ifndef header_1650783109_fluffyvm_h
#define header_1650783109_fluffyvm_h

#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>

#include "config.h"
#include "value.h"
#include "foxgc.h"
#include "collections/list.h"

// X macro idea is awsome
#define FLUFFYVM_STATIC_STRINGS \
  X(outOfMemory, "out of memory") \
  X(outOfMemoryWhileAnErrorOccured, "out of memory while another error occured") \
  X(strtodDidNotProcessAllTheData, "strtod did not process all the data") \
  X(typenames_nil, "nil") \
  X(typenames_string, "string") \
  X(typenames_doubleNum, "double") \
  X(typenames_longNum, "long") \
  X(typenames_table, "table") \
  X(typenames_closure, "function") \
  X(typenames_light_userdata, "userdata") \
  X(typenames_full_userdata, "userdata") \
  X(typenames_garbage_collectable_userdata, "userdata") \
  X(typenames_bool, "boolean") \
  X(typenames_coroutine, "coroutine") \
  X(invalidCapacity, "invalid capacity") \
  X(badKey, "bad key") \
  X(protobufFailedToUnpackData, "protobuf failed to unpack data") \
  X(unsupportedBytecode, "unsupported bytecode version") \
  X(pthreadCreateError, "pthread_create call unsuccessful") \
  X(invalidBytecode, "invalid bytecode") \
  X(cannotResumeDeadCoroutine, "cannot resume dead corotine") \
  X(cannotResumeRunningCoroutine, "cannot resume running coroutine") \
  X(cannotSuspendTopLevelCoroutine, "cannot suspend top level coroutine") \
  X(stackOverflow, "stack overflow") \
  X(stackUnderflow, "stack underflow") \
  X(notInCoroutine, "not in coroutine") \
  X(coroutineNestTooDeep, "coroutine nest too deep") \
  X(attemptToLoadNonExistentPrototype, "attempt to load not existent prototype") \
  X(attemptToCallNilValue, "attempt to call nil value") \
  X(invalidRemoveCall, "invalid argument for interpreter_remove") \
  X(invalidStackIndex, "invalid stack index for C function (lua C API compatibility layer)") \
  X(bool_true, "true") \
  X(bool_false, "false") \
  X(nativeFunctionExplicitlyDisabledYieldingForThisCoroutine, "A native function explicitly disabled yielding for this coroutine") \
  X(attemptToXmoveOnRunningCoroutine, "attempt to xmove on running coroutine") \
  X(attemptToXmoveOnDeadCoroutine, "attempt to xmove on dead coroutine")
  
/*
  X(illegalInstruction, "illegal instruction") \
  X(invalidArrayBound, "invalid array bound") \
  X(attemptToIndexNonIndexableValue, "attempt to index not indexable value") \
  X(attemptToCallNonCallableValue, "attempt to call not callable value") \
  X(expectNonZeroGotNegative, "expect non zero got negative") \
  X(expectLongOrDoubleOrString, "expect long or double or string") \
*/

struct fluffyvm {
  foxgc_heap_t* heap;
  
  // Essentially scratch pad
  // For each thread to avoid
  // lock contention on single 
  // global root
  pthread_key_t currentThreadRootKey;
  
  atomic_int numberOfManagedThreads;
  
  // Static data for each component
  struct hashtable_static_data* hashTableStaticData;
  struct bytecode_static_data* bytecodeStaticData;
  struct coroutine_static_data* coroutineStaticData;
  struct closure_static_data* closureStaticData;
  struct stack_static_data* stackStaticData;
  struct compat_layer_lua54_static_data* compatLayerLua54StaticData;
  struct string_cache_static_data* stringCacheStaticData;
  
  foxgc_root_t* staticDataRoot;

  // Essentially like errno
  pthread_key_t errMsgKey;
  pthread_key_t errMsgRootRefKey;

  // Keep track of coroutine nesting
  pthread_key_t coroutinesStack;

  atomic_int currentAvailableThreadID;
  pthread_key_t currentThreadID;

  pthread_rwlock_t globalTableLock;
  struct value globalTable;
  foxgc_root_reference_t* globalTableRootRef;

  struct fluffyvm_coroutine* mainThread;

  bool hasInit;
  struct string_cache* stringCache;

  bool hasCachePollerStarted;
  volatile atomic_bool shuttingDown;
  pthread_t stringCachePoller;

 
  struct {
    struct {
      int moduleID;
      struct {
        int userdata;
      } type;
    } compatLayer_Lua54;

    struct {
      int moduleID;
      struct {
        int softReference;
      } type;
    } stringCache;
  } modules; 

  // Static strings
  struct { 
#   define X(name, ...) struct value name;\
                        foxgc_root_reference_t* name ## RootRef;
    FLUFFYVM_STATIC_STRINGS
#   undef X
  } staticStrings;
};

// errmsg in all docs refer to these two functions
// unless noted
void fluffyvm_set_errmsg(struct fluffyvm* vm, struct value val);
struct value fluffyvm_get_errmsg(struct fluffyvm* vm);
bool fluffyvm_is_errmsg_present(struct fluffyvm* vm);
#define fluffyvm_clear_errmsg(vm) fluffyvm_set_errmsg(vm, value_not_present)

typedef void* (^fluffyvm_thread_routine_t)(void*);

// Create new VM and set current thread
// as managed thread
struct fluffyvm* fluffyvm_new(struct foxgc_heap* heap);

bool fluffyvm_is_managed(struct fluffyvm* this);

void fluffyvm_set_global(struct fluffyvm* this, struct value val);
struct value fluffyvm_get_global(struct fluffyvm* this);

uintptr_t fluffyvm_get_owner_key();

ATTRIBUTE((format(printf, 2, 3)))
void fluffyvm_set_errmsg_printf(struct fluffyvm* vm, const char* fmt, ...);
void fluffyvm_set_errmsg_vprintf(struct fluffyvm* vm, const char* fmt, va_list list);

//////////////////////////////////////////////////
// Calls below this line will call abort        //
// if the caller thread is not managed thread   //
//////////////////////////////////////////////////

// Use this to start thread if you
// want new thread capable of using
// VM API calls
bool fluffyvm_start_thread(struct fluffyvm* this, pthread_t* newthread, pthread_attr_t* attr, fluffyvm_thread_routine_t routine, void* args);
foxgc_root_t* fluffyvm_get_root(struct fluffyvm* this);
int fluffyvm_get_thread_id(struct fluffyvm* this);

struct fluffyvm_coroutine* fluffyvm_get_executing_coroutine(struct fluffyvm* this);

void fluffyvm_pop_current_coroutine(struct fluffyvm* this);
bool fluffyvm_push_current_coroutine(struct fluffyvm* this, struct fluffyvm_coroutine* co);

// WARNING: Make sure at the moment you call
// this there are no access to this in the future
void fluffyvm_free(struct fluffyvm* this);

#endif





