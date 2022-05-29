#define FLUFFYVM_INTERNAL

#include <pthread.h>
#include <setjmp.h>
#include <stdatomic.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <Block.h>
#include <inttypes.h>
#include <pthread.h>

#include "closure.h"
#include "fiber.h"
#include "fluffyvm.h"
#include "fluffyvm_types.h"
#include "coroutine.h"
#include "config.h"
#include "interpreter.h"
#include "stack.h"
#include "util/util.h"
#include "value.h"

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

static struct value nilRegisters[FLUFFYVM_REGISTERS_NUM] = {};
static pthread_once_t nilRegistersOnce = PTHREAD_ONCE_INIT;

static void nilRegistersInit() {
  struct value nilValue = value_nil();
  for (int i = 0; i < FLUFFYVM_REGISTERS_NUM; i++)
    value_copy(&nilRegisters[i], &nilValue);
}

bool coroutine_init(struct fluffyvm* vm) {
  pthread_once(&nilRegistersOnce, nilRegistersInit);

  vm->coroutineStaticData = malloc(sizeof(*vm->coroutineStaticData));
  if (!vm->coroutineStaticData)
    return false;
  
  create_descriptor(desc_coroutine, struct fluffyvm_coroutine, {
    offsetof(struct fluffyvm_coroutine, gc_this),
    offsetof(struct fluffyvm_coroutine, gc_stack),
    offsetof(struct fluffyvm_coroutine, thrownedErrorObject)
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
}

void coroutine_function_epilog(struct fluffyvm* vm) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(vm);
  assert(co);

  interpreter_function_epilog(vm, co);

  pthread_mutex_lock(&co->callStackLock);
  internal_function_epilog(vm, co);
  pthread_mutex_unlock(&co->callStackLock);
}

void coroutine_function_epilog_no_lock(struct fluffyvm* vm) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(vm);
  assert(co);

  interpreter_function_epilog(vm, co);
  internal_function_epilog(vm, co);
}

struct fluffyvm_call_state* coroutine_function_prolog(struct fluffyvm* vm, struct fluffyvm_closure* func) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(vm);
  assert(co);
  
  foxgc_root_reference_t* tmpRootRef2 = NULL;
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), &tmpRootRef2, vm->coroutineStaticData->desc_callState, NULL);
  if (!obj)
    goto no_memory;
  
  pthread_mutex_lock(&co->callStackLock);
  if (!stack_push(vm, co->callStack, obj))
    goto error;
  pthread_mutex_unlock(&co->callStackLock);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmpRootRef2); 

  struct fluffyvm_call_state* callState = foxgc_api_object_get_data(obj);
  callState->sp = 0;
  foxgc_api_write_field(obj, 0, obj);
  foxgc_api_write_field(obj, 2, func->gc_this);
  foxgc_api_write_field(obj, 3, co->gc_this);
  callState->closure = func;
  callState->owner = co;

  callState->nativeDebugInfo.funcName = NULL;
  callState->nativeDebugInfo.source = NULL;
  callState->nativeDebugInfo.line = -1;

  foxgc_root_reference_t* tmpRootRef = NULL;
  foxgc_object_t* tmp = NULL;
  
  foxgc_api_write_field(callState->gc_this, 1, NULL);
  
  if (!callState->closure->func) {
    // Allocate register objects array
    // to store value which contain GC object
    tmp = foxgc_api_new_array(vm->heap, fluffyvm_get_root(vm), &tmpRootRef, FLUFFYVM_REGISTERS_NUM, NULL);
    if (!tmp)
      goto no_memory;
    foxgc_api_write_field(callState->gc_this, 1, tmp);
    callState->registersObjectArray = foxgc_api_object_get_data(tmp);
    foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmpRootRef);

    memcpy(callState->registers, nilRegisters, sizeof(callState->registers));
  }

  // Allocate register objects stack
  // to store value which contain GC object
  tmp = foxgc_api_new_array(vm->heap, fluffyvm_get_root(vm), &tmpRootRef, FLUFFYVM_GENERAL_STACK_SIZE, NULL);
  if (!tmp)
    goto no_memory;
  foxgc_api_write_field(callState->gc_this, 4, tmp);
  callState->generalObjectStack = foxgc_api_object_get_data(tmp);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmpRootRef);
  
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
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->coroutineStaticData->desc_coroutine, ^void (foxgc_object_t* obj) {
    struct fluffyvm_coroutine* this = foxgc_api_object_get_data(obj);
    // I have no clue how this->fiber be null
    if (this->fiber)
      fiber_free(this->fiber);
  });

  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }
  struct fluffyvm_coroutine* this = foxgc_api_object_get_data(obj);
  foxgc_api_write_field(obj, 0, obj);
  
  foxgc_root_reference_t* stackRootRef = NULL;
  this->callStack = stack_new(vm, &stackRootRef, FLUFFYVM_CALL_STACK_SIZE);
  this->owner = vm;
  if (!this->callStack)
    goto error;
  foxgc_api_write_field(obj, 1, this->callStack->gc_this);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), stackRootRef);

  fluffyvm_push_current_coroutine(vm, this);
  if (!coroutine_function_prolog(vm, func)) {
    fluffyvm_pop_current_coroutine(vm);
    goto error;
  }
  fluffyvm_pop_current_coroutine(vm);

  this->isYieldable = true;
  this->isNativeThread = false;
  this->hasError = false;
  this->errorHandler = NULL;
  this->fiber = fiber_new(Block_copy(^void () {
    jmp_buf buf;
    if (setjmp(buf)) {
      struct value errMsg = fluffyvm_get_errmsg(vm);
      value_copy(&this->thrownedError, &errMsg);
      foxgc_api_write_field(this->gc_this, 1, value_get_object_ptr(errMsg));
      this->hasError = true;
  
      this->errorHandler = NULL;
      return;
    }
  
    this->errorHandler = &buf;
    interpreter_exec(vm, this);
    coroutine_function_epilog(vm); 
    this->errorHandler = NULL;
  }));

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
  if (!fluffyvm_push_current_coroutine(vm, co)) 
    return false;
  
  fiber_state_t prevState;
  bool res = fiber_resume(co->fiber, &prevState);
  
  if (!res) {
    switch (prevState) {
      case FIBER_RUNNING: 
        fluffyvm_set_errmsg(vm, vm->staticStrings.cannotResumeRunningCoroutine);
        break;
      case FIBER_DEAD: 
        fluffyvm_set_errmsg(vm, vm->staticStrings.cannotResumeDeadCoroutine);
        break;
      case FIBER_SUSPENDED:
        abort();
    }

    return false;
  }

  fluffyvm_pop_current_coroutine(vm);
  return !co->hasError;
}

bool coroutine_yield(struct fluffyvm* vm) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(vm);
  if (!co) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.notInCoroutine);
    return false;
  }
  
  if (co->isNativeThread) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.cannotSuspendTopLevelCoroutine);
    return false;
  }
  
  if (!co->isYieldable) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.nativeFunctionExplicitlyDisabledYieldingForThisCoroutine);
    return false;
  }

  return fiber_yield(co->fiber);
}

void coroutine_disallow_yield(struct fluffyvm* vm) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(vm);
  if (!co)
    return;
  
  co->isYieldable = false;
}

void coroutine_allow_yield(struct fluffyvm* vm) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(vm);
  if (!co)
    return;

  co->isYieldable = true;
}

void coroutine_iterate_call_stack(struct fluffyvm* vm, struct fluffyvm_coroutine* co, bool backward, consumer_t consumer) {
  pthread_mutex_lock(&co->callStackLock);
  int usage = co->callStack->sp;
  for (int i = 0; i < usage; i++) {
    int pos = backward ? usage - i - 1 : i;
    struct fluffyvm_call_state* callState = foxgc_api_object_get_data(co->callStack->stack[pos]);
    
    struct fluffyvm_call_frame frame = {
      .isNative = callState->closure->isNative,
      .closure = callState->closure,
      .source = NULL,
      .isMain = pos == 0,

      .line = -1,
      .bytecode = NULL,
      .name = NULL,
      .prototype = NULL
    };

    if (frame.isNative) {
      util_asprintf((char**) &frame.name, "0x%08" PRIXPTR, (uintptr_t) callState->closure->func);
      if (!callState->nativeDebugInfo.source)
        frame.source = "[Native]";
      else
        util_asprintf((char**) &frame.source, "[Native %s]", callState->nativeDebugInfo.source);
      
      if (callState->nativeDebugInfo.line > 0)
        frame.line = callState->nativeDebugInfo.line;

      if (callState->nativeDebugInfo.funcName)
        frame.name = callState->nativeDebugInfo.funcName;
    } else { 
      // I couldn't find any solution 
      // that give this meaningful name
      // for function defined in global
      frame.name = "lua function";
      frame.bytecode = callState->closure->prototype->bytecode;
      frame.prototype = callState->closure->prototype;
      frame.source = value_get_string(frame.prototype->sourceFile);
      
      if (frame.prototype->lineInfo) 
        if (callState->pc < frame.prototype->lineInfo_len)
          frame.line = frame.prototype->lineInfo[callState->pc];
    }

    if (callState->closure->prototype)
      frame.bytecode = callState->closure->prototype->bytecode;
    
    bool res = consumer(&frame);
    
    if (frame.isNative) {
      if (!callState->nativeDebugInfo.funcName)
        free((void*) frame.name);
    }

    if (!res)
      goto exit_function;
  }
  
  exit_function:
  pthread_mutex_unlock(&co->callStackLock);
}

void coroutine_set_debug_info(struct fluffyvm* vm, const char* source, const char* funcName, int line) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(vm);
  co->currentCallState->nativeDebugInfo.funcName = funcName;
  co->currentCallState->nativeDebugInfo.source = source;
  co->currentCallState->nativeDebugInfo.line = line;
}

