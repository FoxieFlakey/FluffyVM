#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "closure.h"
#include "fluffyvm.h"
#include "fluffyvm_types.h"
#include "coroutine.h"
#include "config.h"
#include "interpreter.h"
#include "stack.h"

#define create_descriptor(name, structure, ...) do { \
  size_t offsets[] = __VA_ARGS__; \
  vm->coroutineStaticData->name = foxgc_api_descriptor_new(vm->heap, sizeof(offsets) / sizeof(offsets[0]), offsets, sizeof(structure)); \
  if (vm->coroutineStaticData->name == NULL) \
    return false; \
} while (0)

#define free_descriptor(name)do { \
  if (vm->coroutineStaticData->name) \
    foxgc_api_descriptor_remove(vm->coroutineStaticData->name); \
} while(0)

bool coroutine_init(struct fluffyvm* vm) {
  vm->coroutineStaticData = malloc(sizeof(*vm->coroutineStaticData));
  if (!vm->coroutineStaticData)
    return false;
  
  create_descriptor(desc_coroutine, struct fluffyvm_coroutine, {
    offsetof(struct fluffyvm_coroutine, gc_this),
    offsetof(struct fluffyvm_coroutine, gc_stack)
  });
  
  create_descriptor(desc_callState, struct fluffyvm_call_state, {
    offsetof(struct fluffyvm_call_state, gc_this),
    offsetof(struct fluffyvm_call_state, gc_registerObjectArray),
    offsetof(struct fluffyvm_call_state, gc_closure),
    offsetof(struct fluffyvm_call_state, gc_owner),
    offsetof(struct fluffyvm_call_state, gc_generalObjectStack)
  });

  return true;
}

void coroutine_cleanup(struct fluffyvm* vm) {
  if (!vm->coroutineStaticData)
    return;
  
  free_descriptor(desc_callState);
  free_descriptor(desc_coroutine);
  free(vm->coroutineStaticData);
}

static void internal_function_epilog(struct fluffyvm* vm, struct fluffyvm_coroutine* co) {
  // Abort on underflow
  bool tmp = stack_pop(vm, co->callStack, NULL, NULL);
  assert(tmp);
  if (!stack_peek(vm, co->callStack, (void**) &co->currentCallState))
    co->currentCallState = NULL;
  interpreter_function_epilog(vm, co);
}

void coroutine_function_epilog(struct fluffyvm* vm, struct fluffyvm_coroutine* co) {
  internal_function_epilog(vm, co);
  interpreter_function_epilog(vm, co);
}

struct fluffyvm_call_state* coroutine_function_prolog(struct fluffyvm* vm, struct fluffyvm_coroutine* co, struct fluffyvm_closure* func) {
  foxgc_root_reference_t* tmpRootRef2 = NULL;
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), &tmpRootRef2, vm->coroutineStaticData->desc_callState, NULL);
  if (!obj)
    goto no_memory;
  
  if (!stack_push(vm, co->callStack, obj))
    goto error;
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmpRootRef2); 

  struct fluffyvm_call_state* callState = foxgc_api_object_get_data(obj);
  callState->pc = 0;
  foxgc_api_write_field(obj, 0, obj);
  foxgc_api_write_field(obj, 2, func->gc_this);
  foxgc_api_write_field(obj, 3, co->gc_this);
  callState->closure = func;
  callState->owner = co;

  {
    foxgc_root_reference_t* tmpRootRef = NULL;
    foxgc_object_t* tmp = NULL;
    
    // Allocate register objects array
    // to store value which contain GC object
    tmp = foxgc_api_new_array(vm->heap, fluffyvm_get_root(vm), &tmpRootRef, FLUFFYVM_REGISTERS_NUM, NULL);
    if (!tmp)
      goto no_memory;
    foxgc_api_write_field(callState->gc_this, 1, tmp);
    callState->registersObjectArray = foxgc_api_object_get_data(tmp);
    foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmpRootRef);
  
    // Allocate register objects stack
    // to store value which contain GC object
    tmp = foxgc_api_new_array(vm->heap, fluffyvm_get_root(vm), &tmpRootRef, FLUFFYVM_GENERAL_STACK_SIZE, NULL);
    if (!tmp)
      goto no_memory;
    foxgc_api_write_field(callState->gc_this, 4, tmp);
    callState->generalObjectStack = foxgc_api_object_get_data(tmp);
    foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmpRootRef);
  }

  memset(callState->registers, 0, sizeof(callState->registers));

  co->currentCallState = callState;
  
  if (!interpreter_function_prolog(vm, co, func))
    goto error;
  return callState;
 
  no_memory:
  fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
  error:
  if (obj)
    internal_function_epilog(vm, co);
  return NULL;
}

struct fluffyvm_coroutine* coroutine_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, struct fluffyvm_closure* func) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->coroutineStaticData->desc_coroutine, NULL);
  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }
  struct fluffyvm_coroutine* this = foxgc_api_object_get_data(obj);
  foxgc_api_write_field(obj, 0, obj);
  
  this->state = FLUFFYVM_COROUTINE_STATE_SUSPENDED;
  
  foxgc_root_reference_t* stackRootRef = NULL;
  this->callStack = stack_new(vm, &stackRootRef, FLUFFYVM_CALL_STACK_SIZE);
  if (!this->callStack)
    goto error;
  foxgc_api_write_field(obj, 1, this->callStack->gc_this);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), stackRootRef);

  // Potential memory leak from unknown strong
  // reference to the call state
  if (!coroutine_function_prolog(vm, this, func))
    goto error;
  
  return this;
 
  error:
  /*
  struct fluffyvm_coroutine* callState = NULL;
  stack_peek(vm, this->callStack, (void**) &callState);
  
  // UvU
  if (callState == this) 
    coroutine_function_epilog(vm, this);
  */

  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), *rootRef);
  *rootRef = NULL;
  return NULL;
}

bool coroutine_resume(struct fluffyvm* vm, struct fluffyvm_coroutine* co) {
  if (co->state == FLUFFYVM_COROUTINE_STATE_DEAD) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.cannotResumeDeadCoroutine);
    return false;
  }
  
  if (co->state == FLUFFYVM_COROUTINE_STATE_RUNNING) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.cannotResumeRunningCoroutine);
    return false;
  }

  fluffyvm_set_executing_coroutine(vm, NULL);
  bool res = interpreter_exec(vm, co); 
  fluffyvm_set_executing_coroutine(vm, NULL);
  return res;
}




