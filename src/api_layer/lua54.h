#ifndef header_1653536277_lua54_h
#define header_1653536277_lua54_h

// Compatibility layer for Lua 5.4 C API
// From C API section: https://www.lua.org/manual/5.4/contents.html#index

#ifdef lua_h
# error "Don't include 'lua.h' file; This compat layer serve as replacement for 'lua.h'"
#endif

#include <stdint.h>

#include "types.h"
#include "config.h"

// Types
typedef struct fluffyvm lua_State;
typedef int (*lua_CFunction)(lua_State*);
typedef fluffyvm_integer lua_Integer;
typedef fluffyvm_number lua_Number;
typedef intptr_t lua_KContext;
typedef int (*lua_KFunction)(lua_State* L, int status, lua_KContext ctx);
typedef fluffyvm_unsigned lua_Unsigned;

// Macros
#define FLUFFYVM_DECLARE(ret, name, ...) ret fluffyvm_compat_lua54_ ## name(__VA_ARGS__)
#define LUA_MULTRET (-1)

// Lua status
enum {
  LUA_OK,
  LUA_ERRRUN,
  LUA_ERRMEM,
  LUA_ERRERR,
  LUA_ERRSYNTAX,
  LUA_YIELD,
  LUA_ERRFILE
};

// Pseudo indexes
#define FLUFFYVM_COMPAT_LAYER_PSEUDO_INDEX(loc) (0x80000000 & loc)
#define FLUFFYVM_COMPAT_LAYER_IS_PSEUDO_INDEX(idx) ((idx & 0x80000000) != 0)
#define LUA_REGISTRYINDEX FLUFFYVM_COMPAT_LAYER_PSEUDO_INDEX(0)

// Registry indexes
#define LUA_RIDX_MAINTHREAD (0)
#define LUA_RIDX_GLOBALS (1)

// Declarations
FLUFFYVM_DECLARE(void, lua_call, lua_State* L, int nargs, int nresults); 
FLUFFYVM_DECLARE(int, lua_checkstack, lua_State* L, int n); 
FLUFFYVM_DECLARE(int, lua_gettop, lua_State* L); 
FLUFFYVM_DECLARE(void, lua_copy, lua_State* L, int fromidx, int toidx); 
FLUFFYVM_DECLARE(int, lua_absindex, lua_State* L, int idx); 
FLUFFYVM_DECLARE(void, lua_pop, lua_State* L, int n); 
FLUFFYVM_DECLARE(void, lua_remove, lua_State* L, int idx); 

//FLUFFYVM_DECLARE(void, lua_callk, lua_State* L, int nargs, int nresults); 

#ifndef FLUFFYVM_INTERNAL
# ifdef FLUFFYVM_COMPAT_LAYER_REDIRECT_CALL

# endif

# ifdef FLUFFYVM_API_DEBUG_C_FUNCTION
#   ifndef FLUFFYVM_INSERT_DEBUG_INFO
#     define FLUFFYVM_INSERT_DEBUG_INFO(func, F, ...) do {\
        struct fluffyvm* _vm = (F); \
        coroutine_set_debug_info(_vm, __FILE__, __func__, __LINE__); \
        func(_vm, __VA_ARGS__); \
        coroutine_set_debug_info(_vm, NULL, NULL, -1); \
      } while(0)
#   endif
#   define fluffyvm_compat_lua54_lua_call(F, ...) FLUFFYVM_INSERT_DEBUG_INFO(fluffyvm_compat_lua54_lua_call, F, __VA_ARGS__)
#   define fluffyvm_compat_lua54_lua_checkstack(F, ...) FLUFFYVM_INSERT_DEBUG_INFO(fluffyvm_compat_lua54_lua_checkstack, F, __VA_ARGS__)
# endif
#endif

#endif










