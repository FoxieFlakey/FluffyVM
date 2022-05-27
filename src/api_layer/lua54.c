#include "lua54.h"

#include "../fluffyvm.h"
#include "../coroutine.h"
#include "../config.h"
#include "../interpreter.h"

#define EXPORT ATTRIBUTE((visibility("default")))

EXPORT FLUFFYVM_DECLARE(void, lua_call, lua_State* L, int nargs, int nresults) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  if (nresults < 0 || nargs < 0)
    interpreter_error(L, L->staticStrings.expectNonZeroGotNegative);
  
  int funcPosition = co->currentCallState->sp - nargs - 1;
  if (funcPosition < 0)
    goto error;

  struct value func;
  if (!interpreter_peek(L, co->currentCallState, funcPosition, &func)) 
    goto error;
  
  interpreter_call(L, func, nargs, nresults);

  return;
  error:;
}

EXPORT FLUFFYVM_DECLARE(void, lua_callk, lua_State* L, int nargs, int nresults) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  if (nresults < 0 || nargs < 0)
    interpreter_error(L, L->staticStrings.expectNonZeroGotNegative);
  
}


