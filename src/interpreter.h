#ifndef header_1652092729_interpreter_h
#define header_1652092729_interpreter_h

#include "coroutine.h"
#include "fluffyvm.h"

// I put the interpreter implementation
// seperate from coroutine implementation

bool interpreter_exec(struct fluffyvm* vm, struct fluffyvm_coroutine* co);

#endif

