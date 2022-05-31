#ifndef header_1652092729_interpreter_h
#define header_1652092729_interpreter_h

#include <stddef.h>

#include "coroutine.h"
#include "fluffyvm.h"
#include "util/functional/functional.h"
#include "value.h"
#include "config.h"

#define FLUFFYVM_INTERPRETER_REGISTER_ALWAYS_NIL (0xFFFF)
#define FLUFFYVM_INTERPRETER_REGISTER_ENV (0xFFFE)

// Number of values returned
int interpreter_exec(struct fluffyvm* vm, struct fluffyvm_coroutine* co);

bool interpreter_function_prolog(struct fluffyvm* vm, struct fluffyvm_coroutine* co, struct fluffyvm_closure* func);
void interpreter_function_epilog(struct fluffyvm* vm, struct fluffyvm_coroutine* co);

// Sets errmsg on error
bool interpreter_pop(struct fluffyvm* vm, struct fluffyvm_call_state* callState, struct value* result, foxgc_root_reference_t** rootRef);
bool interpreter_push(struct fluffyvm* vm, struct fluffyvm_call_state* callState, struct value value);
bool interpreter_peek(struct fluffyvm* vm, struct fluffyvm_call_state* callState, int index, struct value* result); 

// Remove at `index` until `index - (count - 1)`
// Very broken do not use
//bool interpreter_remove(struct fluffyvm* vm, struct fluffyvm_call_state* callState, int index, int count);

void interpreter_error(struct fluffyvm* vm, struct value errmsg);

// Return index to last accessible stack entry
// so when do bound check use <= not <
int interpreter_get_top(struct fluffyvm* vm, struct fluffyvm_call_state* callState);

struct value interpreter_get_env(struct fluffyvm* vm, struct fluffyvm_call_state* callState);

// handler is optional (can be NULL)
// Block_release is not called for 
// `thingToExecute` and `handler`
bool interpreter_xpcall(struct fluffyvm* F, runnable_t thingToExecute, runnable_t handler);

// Unprotected call
// that means when error occur it
// can longjmp through C functions
void interpreter_call(struct fluffyvm* F, struct value func, int nargs, int nresults);

/*
#ifndef FLUFFYVM_INTERNAL
#  ifdef FLUFFYVM_DEBUG_C_FUNCTION
#   ifndef FLUFFYVM_INSERT_DEBUG_INFO
#     define FLUFFYVM_INSERT_DEBUG_INFO(func, F, ...) do {\
        struct fluffyvm* _vm = (F); \
        coroutine_set_debug_info(_vm, __FILE__, __func__, __LINE__); \
        func(_vm, __VA_ARGS__); \
        coroutine_set_debug_info(_vm, NULL, NULL, -1); \
      } while(0)
#   endif
#   define interpreter_call(F, ...) FLUFFYVM_INSERT_DEBUG_INFO(interpreter_call, F, __VA_ARGS__)
#   define interpreter_error(F, ...) FLUFFYVM_INSERT_DEBUG_INFO(interpreter_error, F, __VA_ARGS__)
#  endif
#endif
*/

#endif






