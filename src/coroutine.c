#include "fluffyvm.h"
#include "fluffyvm_types.h"
#include "coroutine.h"
#include "config.h"
#include "interpreter.h"

#include <stddef.h>
#include <stdlib.h>

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
    offsetof(struct fluffyvm_call_state, gc_owner)
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

struct fluffyvm_coroutine* coroutine_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, struct fluffyvm_closure* func) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->coroutineStaticData->desc_coroutine, NULL);
  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }
  struct fluffyvm_coroutine* this = foxgc_api_object_get_data(obj);
  foxgc_api_write_field(obj, 0, obj);
  
  foxgc_root_reference_t* stackRootRef = NULL;
  this->callStack = stack_new(vm, &stackRootRef, FLUFFYVM_CALL_STACK_SIZE);
  this->state = FLUFFYVM_COROUTINE_STATE_SUSPENDED;
  if (!this->callStack)
    goto error;
  foxgc_api_write_field(obj, 1, this->callStack->gc_this);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), stackRootRef);

  return this;
 
  error:
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), *rootRef);
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

  return interpreter_exec(vm, co); 
}




