#define FLUFFYVM_INTERNAL

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lua54.h"
#include "types.h"
#include "../fluffyvm.h"
#include "../fluffyvm_types.h"
#include "../coroutine.h"
#include "../config.h"
#include "../interpreter.h"
#include "../util/util.h"

#define EXPORT ATTRIBUTE((visibility("default")))

// Vararg type always `struct value`
// Return false if metamethod cant be called
static bool triggerMetamethod(lua_State* L, const char* name, struct value val, int argCount, int retCount, ...) {
  va_list args;
  va_start(args, retCount);
  for (int i = 0; i < argCount; i++)
    va_arg(args, struct value);
  va_end(args);

  return false;
}

static struct value getValueAtStackIndex(lua_State* L, int idx) {  
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
  if ((nresults < 0 && nresults != LUA_MULTRET) || nargs < 0)
    interpreter_error(L->owner, L->owner->staticStrings.expectNonZeroGotNegative);
  
  struct fluffyvm_call_state* currentCallState = L->currentCallState;
  int funcPosition = currentCallState->sp - nargs - 1;
  struct value func;
  if (!interpreter_peek(L->owner, currentCallState, funcPosition, &func)) 
    goto error;

  interpreter_call(L->owner, func, nargs, nresults);
  
  fluffyvm_compat_lua54_lua_rotate(L, funcPosition + 1, -1);
  fluffyvm_compat_lua54_lua_pop(L, 1);

  //for (int i = 1; i <= fluffyvm_compat_lua54_lua_gettop(L); i++) {
  //  if (fluffyvm_compat_lua54_lua_isstring(L, i))
  //    printf("after call Stack[%d]: '%s'\n", i, fluffyvm_compat_lua54_lua_tostring(L, i));
  //  else
  //    printf("after call Stack[%d]: %s\n", i, fluffyvm_compat_lua54_lua_typename(L, fluffyvm_compat_lua54_lua_type(L, i)));
  //}

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
  if (n < 0)
    interpreter_error(L->owner, L->owner->staticStrings.expectNonZeroGotNegative);

  // the `sp` is pointing to `top + 1`
  // no need to account off by one
  return L->currentCallState->sp + n <= foxgc_api_get_array_length(L->currentCallState->gc_generalObjectStack);
}

EXPORT FLUFFYVM_DECLARE(int, lua_gettop, lua_State* L) {
  return L->currentCallState->sp;
}

EXPORT FLUFFYVM_DECLARE(int, lua_absindex, lua_State* L, int idx) {
  int result = idx;
    
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
  struct fluffyvm_call_state* callState = L->currentCallState;

  int dest = fluffyvm_compat_lua54_lua_absindex(L, toidx);
  struct value sourceValue = getValueAtStackIndex(L, fromidx);  

  value_copy(&callState->generalStack[dest - 1], sourceValue);
  foxgc_api_write_array(callState->gc_generalObjectStack, dest - 1, value_get_object_ptr(sourceValue));
}

EXPORT FLUFFYVM_DECLARE(void, lua_pop, lua_State* L, int count) {
  if (count == 0)
    return;
  
  if (count < 0)
    interpreter_error(L->owner, L->owner->staticStrings.expectNonZeroGotNegative);
 
  for (int i = 0; i < count; i++)
    if (!interpreter_pop(L->owner, L->currentCallState, NULL, NULL))
      interpreter_error(L->owner, L->owner->staticStrings.stackUnderflow);
}

EXPORT FLUFFYVM_DECLARE(void, lua_remove, lua_State* L, int idx) { 
  fluffyvm_compat_lua54_lua_rotate(L, idx, -1);
  fluffyvm_compat_lua54_lua_pop(L, 1);
} 

EXPORT FLUFFYVM_DECLARE(void, lua_pushnil, lua_State* L) {
  struct fluffyvm_call_state* callState = L->currentCallState;
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  interpreter_push(L->owner, callState, value_nil()); 
}

FLUFFYVM_DECLARE(const char*, lua_pushstring, lua_State* L, const char* s) { 
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  struct fluffyvm_call_state* callState = L->currentCallState;
  
  foxgc_root_reference_t* tmpRootRef = NULL;
  struct value string = value_new_string(L->owner, s, &tmpRootRef);
  interpreter_push(L->owner, callState, string);
  foxgc_api_remove_from_root2(L->owner->heap, fluffyvm_get_root(L->owner), tmpRootRef);
  return s;
}

EXPORT FLUFFYVM_DECLARE(const char*, lua_pushliteral, lua_State* L, const char* s) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  struct fluffyvm_call_state* callState = L->currentCallState;
  
  foxgc_root_reference_t* tmpRootRef = NULL;
  struct value string = value_new_string_constant(L->owner, s, &tmpRootRef);
  interpreter_push(L->owner, callState, string);
  foxgc_api_remove_from_root2(L->owner->heap, fluffyvm_get_root(L->owner), tmpRootRef);
  return s;
}

EXPORT FLUFFYVM_DECLARE(void, lua_error, lua_State* L) {
  struct fluffyvm_call_state* callState = L->currentCallState;
  
  struct value err;
  if (!interpreter_peek(L->owner, callState, fluffyvm_compat_lua54_lua_gettop(L) - 1, &err))
    interpreter_error(L->owner, fluffyvm_get_errmsg(L->owner));
  
  assert(fluffyvm_get_executing_coroutine(L->owner) == L); /* Why throw error from another coroutine??? */
  interpreter_error(L->owner, err);  
}

FLUFFYVM_DECLARE(void, lua_pushvalue, lua_State* L, int idx) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  struct fluffyvm_call_state* callState = L->currentCallState;
  struct value sourceValue = getValueAtStackIndex(L, idx);
  
  interpreter_push(L->owner, callState, sourceValue);
}

FLUFFYVM_DECLARE(const char*, lua_tolstring, lua_State* L, int idx, size_t* len) {
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
  
        value_copy(&callState->generalStack[location], tmp);
        value_copy(&val, tmp);
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
    case FLUFFYVM_TVALUE_GARBAGE_COLLECTABLE_USERDATA:
      return LUA_TLIGHTUSERDATA;
    case FLUFFYVM_TVALUE_NIL:
      return LUA_TNIL;
    case FLUFFYVM_TVALUE_COROUTINE:
      return LUA_TTHREAD;

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

// Return version of Lua C API which L->owner compat
// layer trying to support
EXPORT FLUFFYVM_DECLARE(lua_Number, lua_version, lua_State* L) {
  return 504;
}

EXPORT FLUFFYVM_DECLARE(void, lua_replace, lua_State* L, int idx) {
  fluffyvm_compat_lua54_lua_copy(L, -1, idx);
  fluffyvm_compat_lua54_lua_pop(L, 1);
}

EXPORT FLUFFYVM_DECLARE(int, lua_isboolean, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TBOOLEAN; 
} 

EXPORT FLUFFYVM_DECLARE(int, lua_isfunction, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TFUNCTION; 
}

EXPORT FLUFFYVM_DECLARE(int, lua_iscfunction, lua_State* L, int idx) {
  if (fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TFUNCTION)
    return getValueAtStackIndex(L, idx).data.closure->isNative && getValueAtStackIndex(L, idx).data.closure->luaCFunction != NULL;
  
  return 0;
} 

EXPORT FLUFFYVM_DECLARE(int, lua_isinteger, lua_State* L, int idx) {
  if (fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TNUMBER)
    return getValueAtStackIndex(L, idx).type == FLUFFYVM_TVALUE_LONG;
  
  return 0;
} 

EXPORT FLUFFYVM_DECLARE(int, lua_islightuserdata, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TLIGHTUSERDATA; 
}

EXPORT FLUFFYVM_DECLARE(int, lua_isnil, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TNIL; 
}

EXPORT FLUFFYVM_DECLARE(int, lua_isnone, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TNONE; 
}

EXPORT FLUFFYVM_DECLARE(int, lua_isnoneornil, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_isnil(L, idx) || fluffyvm_compat_lua54_lua_isnone(L, idx); 
}

EXPORT FLUFFYVM_DECLARE(int, lua_isnumber, lua_State* L, int idx) {
  switch (fluffyvm_compat_lua54_lua_type(L, idx)) {
    case LUA_TNUMBER:
      return true;
    case LUA_TSTRING:
      break;
    default:
      return false;
  }

  return value_todouble(L->owner, getValueAtStackIndex(L, idx)).type == FLUFFYVM_TVALUE_DOUBLE;
}

EXPORT FLUFFYVM_DECLARE(int, lua_isstring, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TSTRING || fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TNUMBER; 
}

EXPORT FLUFFYVM_DECLARE(int, lua_istable, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TTABLE; 
}

EXPORT FLUFFYVM_DECLARE(int, lua_isthread, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TTHREAD; 
}

EXPORT FLUFFYVM_DECLARE(int, lua_isuserdata, lua_State* L, int idx) {
  return fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TUSERDATA || fluffyvm_compat_lua54_lua_type(L, idx) == LUA_TLIGHTUSERDATA; 
}

EXPORT FLUFFYVM_DECLARE(int, lua_isyieldable, lua_State* L) {
  return coroutine_can_yield(L);
}

EXPORT FLUFFYVM_DECLARE(void, lua_len, lua_State* L, int idx) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);

  struct value val = getValueAtStackIndex(L, idx);
  if (val.type == FLUFFYVM_TVALUE_STRING) {
    struct value len = value_new_long(L->owner, value_get_len(val));
    interpreter_push(L->owner, L->currentCallState, len);
    return;
  }
  
  if (triggerMetamethod(L, "__len", val, 0, 1))
    return;
  
  if (val.type == FLUFFYVM_TVALUE_TABLE) {
    struct value len = value_new_long(L->owner, value_get_len(val));
    interpreter_push(L->owner, L->currentCallState, len);
    return;
  }
  return;
}

EXPORT FLUFFYVM_DECLARE(void, lua_createtable, lua_State* L, int narr, int nrec) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  foxgc_root_reference_t* rootRef;
  struct value table = value_new_table(L->owner, FLUFFYVM_HASHTABLE_DEFAULT_LOAD_FACTOR, nrec, &rootRef);
  if (table.type == FLUFFYVM_TVALUE_NOT_PRESENT)
    interpreter_error(L->owner, fluffyvm_get_errmsg(L->owner));
  interpreter_push(L->owner, L->currentCallState, table);
  foxgc_api_remove_from_root2(L->owner->heap, fluffyvm_get_root(L->owner), rootRef);
}

EXPORT FLUFFYVM_DECLARE(void, lua_newtable, lua_State* L) {
  fluffyvm_compat_lua54_lua_createtable(L, 0, 0);
}

static int trampoline(struct fluffyvm* F, struct fluffyvm_call_state* callState, void* udata) {
  lua_State* L = fluffyvm_get_executing_coroutine(F);
  assert(fluffyvm_compat_lua54_lua_gettop(L) >= 2);

  int* nresults = (void*) fluffyvm_compat_lua54_lua_topointer(L, -2);
  int nargs = fluffyvm_compat_lua54_lua_tonumber(L, -1);
  fluffyvm_compat_lua54_lua_pop(L, 2);
  
  if (!fluffyvm_compat_lua54_lua_isfunction(L, 1)) {
    fluffyvm_compat_lua54_lua_pushliteral(L, "main function is not present");
    fluffyvm_compat_lua54_lua_error(L);
  }
  
  fluffyvm_compat_lua54_lua_call(L, nargs, LUA_MULTRET);
  if (nresults)
    *nresults = fluffyvm_compat_lua54_lua_gettop(L);
  return 0;
}

EXPORT FLUFFYVM_DECLARE(lua_State*, lua_newthread, lua_State* L) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  foxgc_root_reference_t* coroutineRootRef = NULL;
  
  struct value thread = value_new_coroutine(L->owner, L->owner->compatLayerLua54StaticData->coroutineTrampoline, &coroutineRootRef);
  if (thread.type == FLUFFYVM_TVALUE_NOT_PRESENT)
    interpreter_error(L->owner, fluffyvm_get_errmsg(L->owner));
  
  interpreter_push(L->owner, L->currentCallState, thread);
  foxgc_api_remove_from_root2(L->owner->heap, fluffyvm_get_root(L->owner), coroutineRootRef);
  
  return thread.data.coroutine;
}

EXPORT FLUFFYVM_DECLARE(int, lua_resume, lua_State* L, lua_State* target, int nargs, int* nresults) {
  if (!fluffyvm_compat_lua54_lua_checkstack(target, 8 + nargs))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
 
  fluffyvm_compat_lua54_lua_pushlightuserdata(target, nresults);
  fluffyvm_compat_lua54_lua_pushinteger(target, nargs);
  coroutine_resume(target->owner, target);
  return !LUA_OK;
}

EXPORT FLUFFYVM_DECLARE(void, lua_pushlightuserdata, lua_State* L, void* ptr) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  foxgc_root_reference_t* rootRef;
  struct value data = value_new_light_userdata(L->owner, L->owner->modules.compatLayer_Lua54.moduleID, L->owner->modules.compatLayer_Lua54.type.userdata, ptr, &rootRef, NULL);
  interpreter_push(L->owner, L->currentCallState, data);
  foxgc_api_remove_from_root2(L->owner->heap, fluffyvm_get_root(L->owner), rootRef);
}

EXPORT FLUFFYVM_DECLARE(void, lua_pushinteger, lua_State* L, lua_Integer integer) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  interpreter_push(L->owner, L->currentCallState, value_new_long(L->owner, integer));
}

EXPORT FLUFFYVM_DECLARE(void, lua_pushnumber, lua_State* L, lua_Number number) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  interpreter_push(L->owner, L->currentCallState, value_new_double(L->owner, number));
}

static int cFunctionTrampoline(struct fluffyvm* F, struct fluffyvm_call_state* callState, void* udata) {
  return ((lua_CFunction) udata)(fluffyvm_get_executing_coroutine(F));
}

EXPORT FLUFFYVM_DECLARE(void, lua_pushcfunction, lua_State* L, lua_CFunction f) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  foxgc_root_reference_t* closureRootRef = NULL;
  struct fluffyvm_closure* closure = closure_from_cfunction(L->owner, &closureRootRef, cFunctionTrampoline, f, NULL, L->currentCallState->closure->env);
  if (!closure)
    interpreter_error(L->owner, fluffyvm_get_errmsg(L->owner));

  closure->luaCFunction = f;
  interpreter_push(L->owner, L->currentCallState, value_new_closure(L->owner, closure));
  foxgc_api_remove_from_root2(L->owner->heap, fluffyvm_get_root(L->owner), closureRootRef); 
}

bool fluffyvm_compat_layer_lua54_init(struct fluffyvm* vm) {
  vm->compatLayerLua54StaticData = malloc(sizeof(*vm->compatLayerLua54StaticData));
  if (!vm->compatLayerLua54StaticData)
    return false;
    
  foxgc_root_reference_t* closureRootRef = NULL;
  struct fluffyvm_closure* trampolineClosure = closure_from_cfunction(vm, &closureRootRef, trampoline, NULL, NULL, value_nil());
  if (!trampolineClosure)
    return false;

  struct value tmp = value_not_present();
  value_copy(&trampolineClosure->env, tmp);
  vm->compatLayerLua54StaticData->coroutineTrampoline = trampolineClosure;
  
  vm->modules.compatLayer_Lua54.moduleID = value_get_module_id();
  vm->modules.compatLayer_Lua54.type.userdata = 1;
  return true;
}

void fluffyvm_compat_layer_lua54_cleanup(struct fluffyvm* F) {
  free(F->compatLayerLua54StaticData);
}

struct temp_data {
  foxgc_root_reference_t* rootRef;
  struct value val;
};

EXPORT FLUFFYVM_DECLARE(void, lua_rotate, lua_State* L, int idx, int n) {
  int start = fluffyvm_compat_lua54_lua_absindex(L, idx) - 1;
  int size = fluffyvm_compat_lua54_lua_gettop(L) - start;
  struct fluffyvm_call_state* callState = L->currentCallState;

  util_collections_rotate(size, n, ^void (int idx, void* _data) {
    struct temp_data* data = _data;
    value_copy(&callState->generalStack[start + idx], data->val);
    foxgc_api_write_array(callState->gc_generalObjectStack, start + idx, value_get_object_ptr(data->val));
    foxgc_api_remove_from_root2(L->owner->heap, fluffyvm_get_root(L->owner), data->rootRef);
    free(data);
  }, ^void* (int idx) {
    struct temp_data* data = malloc(sizeof(*data));
    value_copy(&data->val, callState->generalStack[start + idx]);
    foxgc_root_reference_t* tmp;
    foxgc_api_root_add(L->owner->heap, value_get_object_ptr(data->val), fluffyvm_get_root(L->owner), &tmp);
    data->rootRef = tmp;
    return data;
  });
}

EXPORT FLUFFYVM_DECLARE(const char*, lua_pushlstring, lua_State* L, const char* s, size_t len) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  struct fluffyvm_call_state* callState = L->currentCallState;
  
  foxgc_root_reference_t* tmpRootRef = NULL;
  struct value string = value_new_string2(L->owner, s, len, &tmpRootRef);
  interpreter_push(L->owner, callState, string);
  foxgc_api_remove_from_root2(L->owner->heap, fluffyvm_get_root(L->owner), tmpRootRef);
  
  return s;
}

EXPORT FLUFFYVM_DECLARE(void, lua_pushglobaltable, lua_State* L) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);

  struct fluffyvm_call_state* callState = L->currentCallState;  
  interpreter_push(L->owner, callState, callState->closure->env);
}
  
EXPORT FLUFFYVM_DECLARE(int, lua_pushthread, lua_State* L) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  struct fluffyvm_call_state* callState = L->currentCallState;
  struct value string = value_new_coroutine2(L->owner, L);
  interpreter_push(L->owner, callState, string); 
  return L == L->owner->mainThread;
}

EXPORT FLUFFYVM_DECLARE(void, lua_xmove, lua_State* L, lua_State* to, int n) {
  assert(L->owner == to->owner);
  if (n < 0)
    interpreter_error(L->owner, L->owner->staticStrings.expectNonZeroGotNegative);

  if (to->fiber->state == FIBER_RUNNING)
    interpreter_error(L->owner, L->owner->staticStrings.attemptToXmoveOnRunningCoroutine);
  else if (to->fiber->state == FIBER_DEAD)
    interpreter_error(L->owner, L->owner->staticStrings.attemptToXmoveOnDeadCoroutine);
  
  if (n == 0)
    return;

  if (!fluffyvm_compat_lua54_lua_checkstack(to, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);

  int moveStart = fluffyvm_compat_lua54_lua_gettop(L) - n;
  if (moveStart < 0)
    interpreter_error(L->owner, L->owner->staticStrings.stackUnderflow);

  for (int i = 0; i < n; i++)
    interpreter_push(L->owner, to->currentCallState, L->currentCallState->generalStack[moveStart + i]);
  fluffyvm_compat_lua54_lua_pop(L, n);
}

EXPORT FLUFFYVM_DECLARE(lua_State*, lua_tothread, lua_State* L, int idx) {
  struct value val = getValueAtStackIndex(L, idx);
  if (val.type != FLUFFYVM_TVALUE_COROUTINE)
    return NULL;

  return val.data.coroutine;
}

EXPORT FLUFFYVM_DECLARE(int, lua_toboolean, lua_State* L, int idx) {
  struct value val = getValueAtStackIndex(L, idx);
  if (val.type == FLUFFYVM_TVALUE_BOOL && val.data.boolean == false)
    return 0;
  if (val.type == FLUFFYVM_TVALUE_NIL)
    return 0;
  
  return 1;
}

EXPORT FLUFFYVM_DECLARE(lua_CFunction, lua_tocfunction, lua_State* L, int idx) {
  struct value val = getValueAtStackIndex(L, idx);

  if (val.type != FLUFFYVM_TVALUE_CLOSURE)
    return NULL;
  if (!val.data.closure->isNative)
    return NULL;
  if (!val.data.closure->luaCFunction)
    return NULL;

  return val.data.closure->luaCFunction;
}

EXPORT FLUFFYVM_DECLARE(void, lua_pushboolean, lua_State* L, int b) {
  if (!fluffyvm_compat_lua54_lua_checkstack(L, 1))
    interpreter_error(L->owner, L->owner->staticStrings.stackOverflow);
  
  interpreter_push(L->owner, L->currentCallState, value_new_bool(L->owner, (bool) b));
}














