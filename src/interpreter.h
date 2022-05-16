#ifndef header_1652092729_interpreter_h
#define header_1652092729_interpreter_h

#include "coroutine.h"
#include "fluffyvm.h"
#include "value.h"

#define FLUFFYVM_INTERPRETER_REGISTER_ALWAYS_NIL (0xFFFF)
#define FLUFFYVM_INTERPRETER_REGISTER_ENV (0xFFFE)

// I put the interpreter implementation
// seperate from coroutine implementation

bool interpreter_exec(struct fluffyvm* vm, struct fluffyvm_coroutine* co);
bool interpreter_function_prolog(struct fluffyvm* vm, struct fluffyvm_coroutine* co, struct fluffyvm_closure* func);
void interpreter_function_epilog(struct fluffyvm* vm, struct fluffyvm_coroutine* co);

bool interpreter_pop(struct fluffyvm* vm, struct fluffyvm_call_state* callState, struct value* result, foxgc_root_reference_t** rootRef);
bool interpreter_push(struct fluffyvm* vm, struct fluffyvm_call_state* callState, struct value value);

#endif

