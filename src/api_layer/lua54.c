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
  assert(L);
  // Check if its pseudo index
  if (FLUFFYVM_COMPAT_LAYER_IS_PSEUDO_INDEX(idx) && idx > 0) {
    switch (idx) {
      default:
        abort(); /* Unknown pseudo index */
    }
  }

  int index = fluffyvm_compat_lua54_lua_absindex(L, idx) - 1;
  struct value tmp;
  if (!interpreter_peek(L->owner, L->currentCallState, index, &tmp))
    interpreter_error(L->owner, fluffyvm_get_errmsg(L->owner));

  return tmp;
}

EXPORT FLUFFYVM_DECLARE(void, lua_call, lua_State* L, int nargs, int nresults) {
  assert(L);

  if ((nresults < 0 && nresults != LUA_MULTRET) || nargs < 0)
    interpreter_error(L->owner, L->owner->staticStrings.expectNonZeroGotNegative);
  
  struct fluffyvm_call_state* currentCallState = L->currentCallState;
  int funcPosition = currentCallState->sp - nargs - 1;
  int argsPosition = currentCallState->sp - 1;
  struct value func;
  if (!interpreter_peek(L->owner, currentCallState, funcPosition, &func)) 
    goto error;
  
  interpreter_call(L->owner, func, nargs, nresults);

  // Remove args and function
  interpreter_remove(L->owner, currentCallState, argsPosition, nargs + 1);
  return;
  
  error:
  interpreter_error(L->owner, fluffyvm_get_errmsg(L->owner));
  abort();
}

/*
EXPORT FLUFFYVM_DECLARE(void, lua_callk, lua_State* L, int nargs, int nresults) {
  struct fluffyvm_coroutine* L = fluffyvm_get_executing_coroutine(L);
  if (nresults < 0 || nargs < 0)
    interpreter_error(L, L->staticStrings.expectNonZeroGotNegative);
  
}
*/

EXPORT FLUFFYVM_DECLARE(int, lua_checkstack, lua_State* L, int n) {
  assert(L);
  
  if (n < 0)
    interpreter_error(L->owner, L->owner->staticStrings.expectNonZeroGotNegative);

  // the `sp` is pointing to `top + 1`
  // no need to account off by one
  return L->currentCallState->sp + n <= foxgc_api_get_array_length(L->currentCallState->gc_generalObjectStack) ? 1 : 0;
}

EXPORT FLUFFYVM_DECLARE(int, lua_gettop, lua_State* L) {
  assert(L);
  return L->currentCallState->sp;
}

EXPORT FLUFFYVM_DECLARE(int, lua_absindex, lua_State* L, int idx) {
  int result = idx;
  assert(L);
  
  if (idx == 0)
    goto invalid_stack_index;
  
  if (idx < 0) {
    result = L->currentCallState->sp + idx + 1;
    if (result <= 0)
      goto invalid_stack_index;
  }

  if (result > L->currentCallState->sp)
    goto invalid_stack_index;

  return result;
  
  invalid_stack_index:
  interpreter_error(L->owner, L->owner->staticStrings.invalidStackIndex);
  abort();
}


EXPORT FLUFFYVM_DECLARE(void, lua_copy, lua_State* L, int fromidx, int toidx) {
  assert(L);
  struct fluffyvm_call_state* callState = L->currentCallState;

  int dest = fluffyvm_compat_lua54_lua_absindex(L, toidx);
  struct value sourceValue = getValueAtStackIndex(L, fromidx);  

  value_copy(&callState->generalStack[dest - 1], &sourceValue);
  foxgc_api_write_array(callState->gc_generalObjectStack, dest - 1, value_get_object_ptr(sourceValue));
}

EXPORT FLUFFYVM_DECLARE(void, lua_pop, lua_State* L, int count) {
  assert(L);

  if (count == 0)
    return;
  
  if (count < 0)
    interpreter_error(L->owner, L->owner->staticStrings.expectNonZeroGotNegative);

  struct fluffyvm_call_state* callState = L->currentCallState;
  int top = interpreter_get_top(L->owner, callState);
  
  if (!interpreter_remove(L->owner, callState, top, count))
    interpreter_error(L->owner, L->owner->staticStrings.invalidStackIndex);
}

EXPORT FLUFFYVM_DECLARE(void, lua_remove, lua_State* L, int idx) {
  assert(L);
  struct fluffyvm_call_state* callState = L->currentCallState;
  int top = fluffyvm_compat_lua54_lua_absindex(L, idx) - 1;
  
  if (!interpreter_remove(L->owner, callState, top, 1))
    abort(); /* Unreachable due lua_absindex also perform bound check */
}

EXPORT FLUFFYVM_DECLARE(void, lua_pushnil, lua_State* L) {
  assert(L);
  struct fluffyvm_call_state* callState = L->currentCallState;
  
  if (!interpreter_push(L->owner, callState, value_nil()))
    interpreter_error(L->owner, fluffyvm_get_errmsg(L->owner));
}

FLUFFYVM_DECLARE(const char*, lua_pushstring, lua_State* L, const char* s) {
  assert(L);
  struct fluffyvm_call_state* callState = L->currentCallState;
  
  foxgc_root_reference_t* tmpRootRef = NULL;
  struct value string = value_new_string(L->owner, s, &tmpRootRef);
  if (!interpreter_push(L->owner, callState, string))
    interpreter_error(L->owner, fluffyvm_get_errmsg(L->owner));
  foxgc_api_remove_from_root2(L->owner->heap, fluffyvm_get_root(L->owner), tmpRootRef);
  return s;
}

EXPORT FLUFFYVM_DECLARE(const char*, lua_pushliteral, lua_State* L, const char* s) {
  return fluffyvm_compat_lua54_lua_pushstring(L, s);
}

EXPORT FLUFFYVM_DECLARE(void, lua_error, lua_State* L) {
  assert(L);
  struct fluffyvm_call_state* callState = L->currentCallState;
  
  struct value err;
  if (!interpreter_peek(L->owner, callState, fluffyvm_compat_lua54_lua_gettop(L) - 1, &err)) 
    interpreter_error(L->owner, fluffyvm_get_errmsg(L->owner));
  interpreter_error(L->owner, err);  
}

FLUFFYVM_DECLARE(void, lua_pushvalue, lua_State* L, int idx) {
  assert(L);
  struct fluffyvm_call_state* callState = L->currentCallState;
  struct value sourceValue = getValueAtStackIndex(L, idx);

  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  interpreter_push(L->owner, callState, sourceValue);
}

FLUFFYVM_DECLARE(const char*, lua_tolstring, lua_State* L, int idx, size_t* len) {
  assert(L);
  struct fluffyvm_call_state* callState = L->currentCallState;
  
  int location = fluffyvm_compat_lua54_lua_absindex(L, idx) - 1;
  struct value val = callState->generalStack[location];
  switch (val.type) {
    case FLUFFYVM_TVALUE_STRING:
      break;
    case FLUFFYVM_TVALUE_LONG:
    case FLUFFYVM_TVALUE_DOUBLE: 
      {
        foxgc_root_reference_t* tmpRootRef = NULL;
        struct value tmp = value_tostring(L->owner, val, &tmpRootRef);
        if (tmp.type == FLUFFYVM_TVALUE_NOT_PRESENT)
          interpreter_error(L->owner, fluffyvm_get_errmsg(L->owner));
  
        value_copy(&callState->generalStack[location], &tmp);
        value_copy(&val, &tmp);
        foxgc_api_write_array(callState->gc_generalObjectStack, location, value_get_object_ptr(tmp));
        
        foxgc_api_remove_from_root2(L->owner->heap, fluffyvm_get_root(L->owner), tmpRootRef);
        break;
      }
    default:
      interpreter_error(L->owner, L->owner->staticStrings.expectLongOrDoubleOrString);
  }

  if (len)
    *len = value_get_len(val);

  return value_get_string(val);
}

EXPORT FLUFFYVM_DECLARE(const char*, lua_tostring, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_tolstring(L, idx, NULL);
}

FLUFFYVM_DECLARE(const void*, lua_topointer, lua_State* L, int idx) {
  return value_get_unique_ptr(getValueAtStackIndex(L, idx));
}

EXPORT FLUFFYVM_DECLARE(lua_Number, lua_tonumberx, lua_State* L, int idx, int* isnum) {
  struct value sourceValue = getValueAtStackIndex(L, idx);
  struct value number = value_todouble(L->owner, sourceValue);

  if (number.type == FLUFFYVM_TVALUE_NOT_PRESENT) {
    if (isnum)
      *isnum = false;
    return 0;
  }

  if (isnum)
    *isnum = true;
  return number.data.doubleData;
}

EXPORT FLUFFYVM_DECLARE(lua_Number, lua_tonumber, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_tonumberx(L, idx, NULL);
}

EXPORT FLUFFYVM_DECLARE(lua_Integer, lua_tointegerx, lua_State* L, int idx, int* isnum) {
  return (lua_Integer) fluffyvm_compat_lua54_lua_tonumberx(L, idx, isnum);
}

EXPORT FLUFFYVM_DECLARE(lua_Integer, lua_tointeger, lua_State* L, int idx) {
  return (lua_Integer) fluffyvm_compat_lua54_lua_tointegerx(L, idx, NULL);
}

EXPORT FLUFFYVM_DECLARE(void*, lua_touserdata, lua_State* L, int idx) {
  struct value val = getValueAtStackIndex(L, idx);
  if (val.type != FLUFFYVM_TVALUE_FULL_USERDATA && val.type != FLUFFYVM_TVALUE_LIGHT_USERDATA)
    return NULL;

  return val.data.userdata->data;
}

EXPORT FLUFFYVM_DECLARE(int, lua_type, lua_State* L, int idx) {
  assert(L);
  struct fluffyvm_call_state* callState = L->currentCallState;
  
  if (idx == 0)
    return LUA_TNONE;
  
  if (idx > callState->sp)
    return LUA_TNONE;
  else if (idx < 0)
    if (callState->sp + idx + 1 < 0)
      return LUA_TNONE;

  struct value val = getValueAtStackIndex(L, idx);
  switch (val.type) {
    case FLUFFYVM_TVALUE_STRING:
      return LUA_TSTRING;
    case FLUFFYVM_TVALUE_LONG:
    case FLUFFYVM_TVALUE_DOUBLE:
      return LUA_TNUMBER;
    case FLUFFYVM_TVALUE_TABLE:
      return LUA_TTABLE;
    case FLUFFYVM_TVALUE_CLOSURE:
      return LUA_TFUNCTION;
    case FLUFFYVM_TVALUE_FULL_USERDATA:
      return LUA_TUSERDATA;
    case FLUFFYVM_TVALUE_BOOL:
      return LUA_TBOOLEAN;
    case FLUFFYVM_TVALUE_LIGHT_USERDATA:
      return LUA_TLIGHTUSERDATA;
    case FLUFFYVM_TVALUE_NIL:
      return LUA_TNIL;

    case FLUFFYVM_TVALUE_LAST:
    case FLUFFYVM_TVALUE_NOT_PRESENT:
      abort();
  }

  abort(); /* No impossible */
}

EXPORT FLUFFYVM_DECLARE(const char*, lua_typename, lua_State* L, int type) {
  switch ((lua_Type) type) {
    case LUA_TBOOLEAN:
      return "boolean";
    case LUA_TNIL:
      return "nil";
    case LUA_TNUMBER:
      return "number";
    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
      return "userdata";
    case LUA_TTABLE:
      return "table";
    case LUA_TFUNCTION:
      return "function";
    case LUA_TSTRING:
      return "string";
    case LUA_TTHREAD:
      return "thread";

    case LUA_TNONE:
      break;;
  }

  abort();
}

// Return version of Lua C API which this compat
// layer trying to support
EXPORT FLUFFYVM_DECLARE(lua_Number, lua_version, lua_State* L) {
  return 504;
}

EXPORT FLUFFYVM_DECLARE(void, lua_replace, lua_State* L, int idx) {
  fluffyvm_compat_lua54_lua_copy(L, -1, idx);
  fluffyvm_compat_lua54_lua_pop(L, 1);
}
















