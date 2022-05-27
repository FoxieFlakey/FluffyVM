#define FLUFFYVM_INTERNAL

#include <assert.h>
#include <stdlib.h>

#include "lua54.h"
#include "types.h"
#include "../fluffyvm.h"
#include "../coroutine.h"
#include "../config.h"
#include "../interpreter.h"

#define EXPORT ATTRIBUTE((visibility("default")))

static struct value getValueAtStackIndex(lua_State* L, int idx) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  assert(co);
  // Check if its pseudo index
  if (FLUFFYVM_COMPAT_LAYER_IS_PSEUDO_INDEX(idx)) {
    switch (idx) {
      default:
        abort(); /* Unknown pseudo index */
    }
  }

  int index = fluffyvm_compat_lua54_lua_absindex(L, idx);
  struct value tmp;
  if (!interpreter_peek(L, co->currentCallState, index, &tmp))
    interpreter_error(L, fluffyvm_get_errmsg(L));

  return tmp;
}

EXPORT FLUFFYVM_DECLARE(void, lua_call, lua_State* L, int nargs, int nresults) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  assert(co);

  if ((nresults < 0 && nresults != LUA_MULTRET) || nargs < 0)
    interpreter_error(L, L->staticStrings.expectNonZeroGotNegative);
  
  struct fluffyvm_call_state* currentCallState = co->currentCallState;
  int funcPosition = currentCallState->sp - nargs - 1;
  int argsPosition = currentCallState->sp - 1;
  struct value func;
  if (!interpreter_peek(L, currentCallState, funcPosition, &func)) 
    goto error;
  
  interpreter_call(L, func, nargs, nresults);

  // Remove args and function
  interpreter_remove(L, currentCallState, argsPosition, nargs + 1);
  return;
  
  error:
  interpreter_error(L, fluffyvm_get_errmsg(L));
  abort();
}

/*
EXPORT FLUFFYVM_DECLARE(void, lua_callk, lua_State* L, int nargs, int nresults) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  if (nresults < 0 || nargs < 0)
    interpreter_error(L, L->staticStrings.expectNonZeroGotNegative);
  
}
*/

EXPORT FLUFFYVM_DECLARE(int, lua_checkstack, lua_State* L, int n) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  assert(co);

  // the `sp` is pointing to `top + 1`
  // no need to account off by one
  return co->currentCallState->sp + n <= foxgc_api_get_array_length(co->currentCallState->gc_generalObjectStack) ? 1 : 0;
}

EXPORT FLUFFYVM_DECLARE(int, lua_gettop, lua_State* L) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  assert(co);
  return co->currentCallState->sp;
}

EXPORT FLUFFYVM_DECLARE(int, lua_absindex, lua_State* L, int idx) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  int result = idx;
  assert(co);
  
  if (idx == 0)
    goto invalid_stack_index;
  
  if (idx < 0) {
    result = co->currentCallState->sp + idx + 1;
    if (result <= 0)
      goto invalid_stack_index;
  }

  if (result > co->currentCallState->sp)
    goto invalid_stack_index;

  return result;

  invalid_stack_index:
  interpreter_error(L, L->staticStrings.invalidStackIndex);
  abort();
}


EXPORT FLUFFYVM_DECLARE(void, lua_copy, lua_State* L, int fromidx, int toidx) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  assert(co);
  struct fluffyvm_call_state* callState = co->currentCallState;

  int src = fluffyvm_compat_lua54_lua_absindex(L, fromidx);
  int dest = fluffyvm_compat_lua54_lua_absindex(L, toidx);
    
  value_copy(&callState->generalStack[dest - 1], &callState->generalStack[src - 1]);
  foxgc_api_write_array(callState->gc_generalObjectStack, dest - 1, callState->generalObjectStack[src - 1]);
}

EXPORT FLUFFYVM_DECLARE(void, lua_pop, lua_State* L, int count) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  assert(co);

  if (count == 0)
    return;
  
  if (count < 0)
    interpreter_error(L, L->staticStrings.expectNonZeroGotNegative);

  struct fluffyvm_call_state* callState = co->currentCallState;
  int top = interpreter_get_top(L, callState);
  
  if (!interpreter_remove(L, callState, top, count))
    interpreter_error(L, L->staticStrings.invalidStackIndex);
}

EXPORT FLUFFYVM_DECLARE(void, lua_remove, lua_State* L, int idx) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  assert(co);
  struct fluffyvm_call_state* callState = co->currentCallState;
  int top = fluffyvm_compat_lua54_lua_absindex(L, idx) - 1;
  
  if (!interpreter_remove(L, callState, top, 1))
    abort(); /* Unreachable due lua_absindex also perform bound check */
}




