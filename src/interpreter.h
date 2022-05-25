#ifndef header_1652092729_interpreter_h
#define header_1652092729_interpreter_h

#include "coroutine.h"
#include "fluffyvm.h"
#include "util/functional/functional.h"
#include "value.h"

#define FLUFFYVM_INTERPRETER_REGISTER_ALWAYS_NIL (0xFFFF)
#define FLUFFYVM_INTERPRETER_REGISTER_ENV (0xFFFE)

void interpreter_exec(struct fluffyvm* vm, struct fluffyvm_coroutine* co);
bool interpreter_function_prolog(struct fluffyvm* vm, struct fluffyvm_coroutine* co, struct fluffyvm_closure* func);
void interpreter_function_epilog(struct fluffyvm* vm, struct fluffyvm_coroutine* co);

bool interpreter_pop(struct fluffyvm* vm, struct fluffyvm_call_state* callState, struct value* result, foxgc_root_reference_t** rootRef);
bool interpreter_push(struct fluffyvm* vm, struct fluffyvm_call_state* callState, struct value value);
bool interpreter_peek(struct fluffyvm* vm, struct fluffyvm_call_state* callState, int index, struct value* result); 
void interpreter_error(struct fluffyvm* vm, struct fluffyvm_call_state* callState, struct value errmsg);

// Return index to last accessible stack entry
// so when do bound check use <= not <
int interpreter_get_top(struct fluffyvm* vm, struct fluffyvm_call_state* callState);

struct value interpreter_get_env(struct fluffyvm* vm, struct fluffyvm_call_state* callState);

// handler is optional (can be NULL)
// Block_release is not called for 
// `thingToExecute` and `handler`
bool interpreter_xpcall(struct fluffyvm* F, runnable_t thingToExecute, runnable_t handler);

#endif

