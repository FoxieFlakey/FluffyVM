#ifndef header_1651987028_thread_h
#define header_1651987028_thread_h

#include <pthread.h>
#include <stdatomic.h>
#include <setjmp.h>

#include "api_layer/types.h"
#include "closure.h"
#include "bytecode.h"
#include "util/functional/functional.h"
#include "value.h"
#include "foxgc.h"
#include "stack.h"
#include "config.h"
#include "fiber.h"

// Currently executing function
struct fluffyvm_call_state {
  struct fluffyvm_closure* closure;
  struct fluffyvm_coroutine* owner;
  
  struct value generalStack[FLUFFYVM_GENERAL_STACK_SIZE];
  foxgc_object_t** generalObjectStack;

  struct value registers[FLUFFYVM_REGISTERS_NUM];

  int pc;
  int sp;

  struct {
    const char* source;
    const char* funcName;
    int line;
  } nativeDebugInfo;

  foxgc_object_t** registersObjectArray;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_registerObjectArray;
  foxgc_object_t* gc_closure;
  foxgc_object_t* gc_owner;
  foxgc_object_t* gc_generalObjectStack;
};

struct fluffyvm_coroutine {
  bool isNativeThread;
  bool isYieldable;

  struct fiber* fiber;
  struct fluffyvm_call_state* currentCallState;
  jmp_buf* errorHandler;

  pthread_mutex_t callStackLock;
  struct fluffyvm_stack* callStack;
  
  bool hasError;
  struct value thrownedError;
  foxgc_object_t* thrownedErrorObject;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_stack;
};

struct fluffyvm_call_frame {
  bool isNative;

  const char* name;
  const char* source;
  struct fluffyvm_closure* closure;

  int line;
  
  struct fluffyvm_bytecode* bytecode;
  struct fluffyvm_prototype* prototype;
};

bool coroutine_init(struct fluffyvm* vm);
void coroutine_cleanup(struct fluffyvm* vm);

struct fluffyvm_coroutine* coroutine_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, struct fluffyvm_closure* func);

bool coroutine_yield(struct fluffyvm* vm);
bool coroutine_resume(struct fluffyvm* vm, struct fluffyvm_coroutine* coroutine);

struct fluffyvm_call_state* coroutine_function_prolog(struct fluffyvm* vm, struct fluffyvm_closure* func);
void coroutine_function_epilog(struct fluffyvm* vm);
void coroutine_function_epilog_no_lock(struct fluffyvm* vm);

void coroutine_disallow_yield(struct fluffyvm* vm);
void coroutine_allow_yield(struct fluffyvm* vm);

// Iterates the call stack
// Note: Do not store pointer of the call frame
//       it is stack allocated
void coroutine_iterate_call_stack(struct fluffyvm* vm, struct fluffyvm_coroutine* co, bool backward, consumer_t consumer);

// This function is for assisting debugging native
// modules
void coroutine_set_debug_info(struct fluffyvm* vm, const char* source, const char* funcName, int line);

#ifndef FLUFFYVM_INTERNAL
# ifdef FLUFFYVM_DEBUG_C_FUNCTION
#   define coroutine_iterate_call_stack(F, ...) do {\
      struct fluffyvm* _vm = (F); \
      if (fluffyvm_get_executing_coroutine(_vm)) { \
        coroutine_set_debug_info(_vm, __FILE__, __func__, __LINE__); \
      } \
      coroutine_iterate_call_stack(_vm, __VA_ARGS__); \
      if (fluffyvm_get_executing_coroutine(_vm)) { \
        coroutine_set_debug_info(_vm, NULL, NULL, -1); \
      } \
    } while(0)
# endif
#endif

#endif



