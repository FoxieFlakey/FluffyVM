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
  if (FLUFFYVM_COMPAT_LAYER_IS_PSEUDO_INDEX(idx) && idx > 0) {
    switch (idx) {
      default:
        abort(); /* Unknown pseudo index */
    }
  }

  int index = fluffyvm_compat_lua54_lua_absindex(L, idx) - 1;
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
  
  if (n < 0)
    interpreter_error(L, L->staticStrings.expectNonZeroGotNegative);

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

  int dest = fluffyvm_compat_lua54_lua_absindex(L, toidx);
  struct value sourceValue = getValueAtStackIndex(L, fromidx);  

  value_copy(&callState->generalStack[dest - 1], &sourceValue);
  foxgc_api_write_array(callState->gc_generalObjectStack, dest - 1, value_get_object_ptr(sourceValue));
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

EXPORT FLUFFYVM_DECLARE(void, lua_pushnil, lua_State* L) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  assert(co);
  struct fluffyvm_call_state* callState = co->currentCallState;
  
  if (!interpreter_push(L, callState, value_nil()))
    interpreter_error(L, fluffyvm_get_errmsg(L));
}

FLUFFYVM_DECLARE(const char*, lua_pushstring, lua_State* L, const char* s) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  assert(co);
  struct fluffyvm_call_state* callState = co->currentCallState;
  
  foxgc_root_reference_t* tmpRootRef = NULL;
  struct value string = value_new_string(L, s, &tmpRootRef);
  if (!interpreter_push(L, callState, string))
    interpreter_error(L, fluffyvm_get_errmsg(L));
  foxgc_api_remove_from_root2(L->heap, fluffyvm_get_root(L), tmpRootRef);
  return s;
}

EXPORT FLUFFYVM_DECLARE(const char*, lua_pushliteral, lua_State* L, const char* s) {
  return fluffyvm_compat_lua54_lua_pushstring(L, s);
}

EXPORT FLUFFYVM_DECLARE(void, lua_error, lua_State* L) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  assert(co);
  struct fluffyvm_call_state* callState = co->currentCallState;
  
  struct value err;
  if (!interpreter_peek(L, callState, fluffyvm_compat_lua54_lua_gettop(L) - 1, &err)) 
    interpreter_error(L, fluffyvm_get_errmsg(L));
  interpreter_error(L, err);  
}

FLUFFYVM_DECLARE(void, lua_pushvalue, lua_State* L, int idx) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  assert(co);
  struct fluffyvm_call_state* callState = co->currentCallState;
  struct value sourceValue = getValueAtStackIndex(L, idx);

  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L, L->staticStrings.stackOverflow);
  
  interpreter_push(L, callState, sourceValue);
}

FLUFFYVM_DECLARE(const char*, lua_tolstring, lua_State* L, int idx, size_t* len) {
  struct fluffyvm_coroutine* co = fluffyvm_get_executing_coroutine(L);
  assert(co);
  struct fluffyvm_call_state* callState = co->currentCallState;
  
  int location = fluffyvm_compat_lua54_lua_absindex(L, idx) - 1;
  struct value val = callState->generalStack[location];
  switch (val.type) {
    case FLUFFYVM_TVALUE_STRING:
      break;
    case FLUFFYVM_TVALUE_LONG:
    case FLUFFYVM_TVALUE_DOUBLE: 
      {
        foxgc_root_reference_t* tmpRootRef = NULL;
        struct value tmp = value_tostring(L, val, &tmpRootRef);
        if (tmp.type == FLUFFYVM_TVALUE_NOT_PRESENT)
          interpreter_error(L, fluffyvm_get_errmsg(L));
  
        value_copy(&callState->generalStack[location], &tmp);
        value_copy(&val, &tmp);
        foxgc_api_write_array(callState->gc_generalObjectStack, location, value_get_object_ptr(tmp));
        
        foxgc_api_remove_from_root2(L->heap, fluffyvm_get_root(L), tmpRootRef);
        break;
      }
    default:
      interpreter_error(L, L->staticStrings.expectLongOrDoubleOrString);
  }

  if (len)
    *len = value_get_len(val);

  return value_get_string(val);
}

EXPORT FLUFFYVM_DECLARE(const char*, lua_tostring, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_tolstring(L, idx, NULL);
}










