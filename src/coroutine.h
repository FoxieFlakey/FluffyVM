#ifndef header_1651987028_thread_h
#define header_1651987028_thread_h

#include "closure.h"
#include "bytecode.h"
#include "fluffyvm.h"
#include "value.h"
#include "foxgc.h"
#include "stack.h"
#include "config.h"

// Currently executing function
struct fluffyvm_call_state {
  struct fluffyvm_closure* closure;
  struct fluffyvm_coroutine* owner;
  
  struct value generalStack[FLUFFYVM_GENERAL_STACK_SIZE];
  foxgc_object_t* generalObjectStack;

  struct value registers[FLUFFYVM_REGISTERS_NUM];

  int pc;
  int sp;

  foxgc_object_t** registersObjectArray;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_registerObjectArray;
  foxgc_object_t* gc_closure;
  foxgc_object_t* gc_owner;
  foxgc_object_t* gc_generalObjectStack;
};

typedef enum {
  FLUFFYVM_COROUTINE_STATE_RUNNING,
  FLUFFYVM_COROUTINE_STATE_DEAD,
  FLUFFYVM_COROUTINE_STATE_SUSPENDED
} fluffyvm_coroutine_state;

struct fluffyvm_coroutine {
  // If this running on a
  // native thread the coroutine
  // cannot be yielded
  bool isNativeThread;
  fluffyvm_coroutine_state state;
 
  struct fluffyvm_call_state* currentCallState;
  struct fluffyvm_stack* callStack;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_stack;
};

bool coroutine_init(struct fluffyvm* vm);
void coroutine_cleanup(struct fluffyvm* vm);

struct fluffyvm_coroutine* coroutine_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, struct fluffyvm_closure* func);
bool coroutine_resume(struct fluffyvm* vm, struct fluffyvm_coroutine* coroutine);

struct fluffyvm_call_state* coroutine_function_prolog(struct fluffyvm* vm, struct fluffyvm_coroutine* co, struct fluffyvm_closure* func);
void coroutine_function_epilog(struct fluffyvm* vm, struct fluffyvm_coroutine* co);

#endif



