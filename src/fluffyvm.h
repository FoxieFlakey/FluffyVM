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

  foxgc_root_t* staticDataRoot;

  // Essentially like errno
  pthread_key_t errMsgKey;
  pthread_key_t errMsgRootRefKey;

  // Keep track of coroutine nesting
  pthread_key_t coroutinesStack;

  atomic_int currentAvailableThreadID;
  pthread_key_t currentThreadID;

  bool hasInit;

  // Static strings
  struct { 
    struct value invalidCapacity;
    foxgc_root_reference_t* invalidCapacityRootRef;
   
    struct value badKey;
    foxgc_root_reference_t* badKeyRootRef;
    
    struct value outOfMemory;
    foxgc_root_reference_t* outOfMemoryRootRef;
    
    struct value outOfMemoryWhileHandlingError;
    foxgc_root_reference_t* outOfMemoryWhileHandlingErrorRootRef;
    
    struct value outOfMemoryWhileAnErrorOccured;
    foxgc_root_reference_t* outOfMemoryWhileAnErrorOccuredRootRef;
    
    struct value strtodDidNotProcessAllTheData;
    foxgc_root_reference_t* strtodDidNotProcessAllTheDataRootRef;
    
    struct value protobufFailedToUnpackData;
    foxgc_root_reference_t* protobufFailedToUnpackDataRootRef;
    
    struct value pthreadCreateError;
    foxgc_root_reference_t* pthreadCreateErrorRootRef;
    
    struct value unsupportedBytecode;
    foxgc_root_reference_t* unsupportedBytecodeRootRef;
    
    struct value invalidBytecode;
    foxgc_root_reference_t* invalidBytecodeRootRef;

    struct value invalidArrayBound;
    foxgc_root_reference_t* invalidArrayBoundRootRef;
    
    struct value cannotResumeDeadCoroutine;
    foxgc_root_reference_t* cannotResumeDeadCoroutineRootRef;
    
    struct value cannotResumeRunningCoroutine;
    foxgc_root_reference_t* cannotResumeRunningCoroutineRootRef;
    
    struct value cannotSuspendTopLevelCoroutine;
    foxgc_root_reference_t* cannotSuspendTopLevelCoroutineRootRef;
    
    struct value illegalInstruction;
    foxgc_root_reference_t* illegalInstructionRootRef;
    
    struct value stackOverflow;
    foxgc_root_reference_t* stackOverflowRootRef;
    
    struct value stackUnderflow;
    foxgc_root_reference_t* stackUnderflowRootRef;
    
    struct value attemptToIndexNonIndexableValue;
    foxgc_root_reference_t* attemptToIndexNonIndexableValueRootRef;
    
    struct value attemptToCallNonCallableValue;
    foxgc_root_reference_t* attemptToCallNonCallableValueRootRef;
    
    struct value notInCoroutine;
    foxgc_root_reference_t* notInCoroutineRootRef;
    
    struct value coroutineNestTooDeep;
    foxgc_root_reference_t* coroutineNestTooDeepRootRef;

    // Type names
    struct {
      struct value nil;
      foxgc_root_reference_t* nilRootRef;
      
      struct value longNum;
      foxgc_root_reference_t* longNumRootRef;
      
      struct value doubleNum;
      foxgc_root_reference_t* doubleNumRootRef;
      
      struct value string;
      foxgc_root_reference_t* stringRootRef;
      
      struct value table;
      foxgc_root_reference_t* tableRootRef;
      
      struct value closure;
      foxgc_root_reference_t* closureRootRef;
    } typenames;
  } staticStrings;
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





