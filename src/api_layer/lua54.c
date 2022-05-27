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

