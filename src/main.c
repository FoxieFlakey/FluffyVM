#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <Block.h>

#include "config.h"
#include "coroutine.h"
#include "fiber.h"
#include "foxgc.h"
#include "fluffyvm.h"
#include "util/functional/functional.h"
#include "value.h"
#include "hashtable.h"
#include "loader/bytecode/json.h"
#include "util/util.h"
#include "bootloader.h"
#include "closure.h"
#include "interpreter.h"

#include "api_layer/lua54.h"

#define KB (1024)
#define MB (1024 * KB)

static foxgc_heap_t* heap = NULL;

static double toKB(size_t bytes) {
  return ((double) bytes) / KB;
}

static void printMemUsage(const char* msg) {
  puts("------------------------------------");
  puts(msg);
  //puts("------------------------------------");
  printf("Heap Usage: %lf / %lf KiB\n", toKB(foxgc_api_get_heap_usage(heap)), toKB(foxgc_api_get_heap_size(heap)));
  printf("Metaspace: %lf / %lf KiB\n", toKB(foxgc_api_get_metaspace_usage(heap)), toKB(foxgc_api_get_metaspace_size(heap))); 
  puts("------------------------------------");
}
  
static void* bytecodeRaw = NULL;
static size_t bytecodeRawLen = 0;

ATTRIBUTE((format(printf, 1, 2)))
static void collectAndPrintMemUsage(const char* fmt, ...) {
  char* tmp;
  
  va_list args;
  va_start(args, fmt);
  util_vasprintf(&tmp, fmt, args);
  va_end(args);

  //foxgc_api_do_full_gc(heap);
  //foxgc_api_do_full_gc(heap);
  //foxgc_api_do_full_gc(heap);
  //foxgc_api_do_full_gc(heap);
  //foxgc_api_do_full_gc(heap);
  printMemUsage(tmp);
  free(tmp);
}

static void stdlib_print_stacktrace(struct fluffyvm* F, struct fluffyvm_coroutine* co) {
  const int tid = fluffyvm_get_thread_id(F);
  
  printf("[Thread %d] Stacktrace:\n", tid);
  coroutine_iterate_call_stack(F, co, true, ^bool (void* _arg) {
    struct fluffyvm_call_frame* frame = _arg;
    printf("[Thread %d] \t%s:%d: in %s%s\n", tid, frame->source, frame->line, frame->name, frame->isMain ? " (main function)" : "");
    return true;
  });
}

static int stdlib_print(struct fluffyvm* F, struct fluffyvm_call_state* callState, void* udata) {
  const int tid = fluffyvm_get_thread_id(F);
  lua_State* L = fluffyvm_get_executing_coroutine(F);

  for (int i = 0; i < fluffyvm_compat_lua54_lua_gettop(L); i++)
    printf("[Thread %d] Printer: %s\n", tid, fluffyvm_compat_lua54_lua_tostring(L, i + 1));

  stdlib_print_stacktrace(F, callState->owner);
  return 0;
}

static int stdlib_return_string(struct fluffyvm* F, struct fluffyvm_call_state* callState, void* udata) {
  lua_State* L = fluffyvm_get_executing_coroutine(F);

  fluffyvm_compat_lua54_lua_pushstring(L, "Returned from C function (Printed twice) (arg #1)");
  fluffyvm_compat_lua54_lua_pushstring(L, "Does not exist #1");
  fluffyvm_compat_lua54_lua_pushstring(L, "Does not exist #2");
  fluffyvm_compat_lua54_lua_pushstring(L, "Does not exist #3");
  fluffyvm_compat_lua54_lua_pop(L, 1);  
  fluffyvm_compat_lua54_lua_pushstring(L, "Be replaced");

  fluffyvm_compat_lua54_lua_remove(L, -3);
  fluffyvm_compat_lua54_lua_remove(L, -2);
  
  fluffyvm_compat_lua54_lua_pushstring(L, "Returned from C function (Only printed once) (arg #2)");
  fluffyvm_compat_lua54_lua_replace(L, -2);
  
  return fluffyvm_compat_lua54_lua_gettop(L);
}

static int stdlib_call_func(struct fluffyvm* F, struct fluffyvm_call_state* callState, void* udata) {
  lua_State* L = fluffyvm_get_executing_coroutine(F);

  fluffyvm_compat_lua54_lua_call(L, 0, 0);
  return 0;
}

static int stdlib_call_func2(struct fluffyvm* F, struct fluffyvm_call_state* callState, void* udata) {
  lua_State* L = fluffyvm_get_executing_coroutine(F);

  fluffyvm_compat_lua54_lua_pushvalue(L, -1);
  
  fluffyvm_compat_lua54_lua_pushstring(L, "Passing an argument to function (args #1)");
  fluffyvm_compat_lua54_lua_pushstring(L, "Passing an argument to function (args #2)");
  
  fluffyvm_compat_lua54_lua_call(L, 2, 2);
  fluffyvm_compat_lua54_lua_call(L, 2, 0);
  return 0;
}

static int stdlib_error(struct fluffyvm* F, struct fluffyvm_call_state* callState, void* udata) {
  lua_State* L = fluffyvm_get_executing_coroutine(F);

  fluffyvm_compat_lua54_lua_error(L);
  return 0;
}

static void registerCFunction(struct fluffyvm* F, const char* name, closure_cfunction_t cfunc) {
  struct value globalTable = fluffyvm_get_global(F);

  foxgc_root_reference_t* printRootRef = NULL;
  foxgc_root_reference_t* printStringRootRef = NULL;
  struct fluffyvm_closure* printFunc = closure_from_cfunction(F, &printRootRef, cfunc, NULL, NULL, globalTable);
  assert(printFunc);
  
  struct value printVal = value_new_closure(F, printFunc);
  struct value printString = value_new_string(F, name, &printStringRootRef);
  assert(printString.type != FLUFFYVM_TVALUE_NOT_PRESENT);
  
  value_table_set(F, globalTable, printString, printVal);

  foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), printRootRef);
  foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), printStringRootRef);
}

/* Generation sizes guide
 * 1 : 2 : 8
 */
int main2() {
  heap = foxgc_api_new(8 * MB, 16 * MB, 64 * MB,
                                 3, 3, 

                                 256 * KB, 2 * MB,

                                 32 * KB);
  collectAndPrintMemUsage("Before VM creation");

  struct fluffyvm* F = fluffyvm_new(heap);
  if (!F) {
    printf("FATAL: can't create VM\n");
    goto cannotCreateVm;
  }

  collectAndPrintMemUsage("After VM creation but before test");
    
  registerCFunction(F, "print", stdlib_print);
  registerCFunction(F, "return_string", stdlib_return_string);
  registerCFunction(F, "call_func", stdlib_call_func);
  registerCFunction(F, "error", stdlib_error);
  registerCFunction(F, "call_func2", stdlib_call_func2);
  
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  collectAndPrintMemUsage("After global table initialization but before test");
  
  fluffyvm_thread_routine_t test = ^void* (void* _) {
    const int tid = fluffyvm_get_thread_id(F);
    lua_State* L = fluffyvm_get_executing_coroutine(F);

    foxgc_root_reference_t* bytecodeRootRef = NULL;
    struct fluffyvm_bytecode* bytecode = bytecode_loader_json_load(F, &bytecodeRootRef, bytecodeRaw, bytecodeRawLen);
    if (!bytecode)
      goto error; 
    
    foxgc_root_reference_t* closureRootRef = NULL;
    struct fluffyvm_closure* closure = closure_new(F, &closureRootRef, bytecode->mainPrototype, fluffyvm_get_global(F));
    foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), bytecodeRootRef);
    
    if (!closure)
      goto error;

    lua_State* L2 = fluffyvm_compat_lua54_lua_newthread(L);
    struct value val = value_new_closure(F, closure);
    interpreter_push(F, L->currentCallState, val);
    fluffyvm_compat_lua54_lua_xmove(L, L2, 1);
    foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), closureRootRef);
    collectAndPrintMemUsage("[Thread %d] Coroutine created", tid);

    if (fluffyvm_compat_lua54_lua_resume(L, L2, 0, NULL) != LUA_OK) {
      struct value errMsg = L2->thrownedError;
      printf("[Thread %d] Coroutine error: %s\n", tid, value_get_string(errMsg));
      stdlib_print_stacktrace(F, L2);
      goto coroutine_crashed;
    }
    
    fluffyvm_compat_lua54_lua_pop(L, fluffyvm_compat_lua54_lua_gettop(L));
    fluffyvm_clear_errmsg(F);
    collectAndPrintMemUsage("[Thread %d] After test", tid);
    return NULL;

    coroutine_crashed:
    error:
    fluffyvm_compat_lua54_lua_pop(L, fluffyvm_compat_lua54_lua_gettop(L));
    if (fluffyvm_is_errmsg_present(F))
      collectAndPrintMemUsage("[Thread %d] Error: %s", tid, value_get_string(fluffyvm_get_errmsg(F)));
    fluffyvm_clear_errmsg(F);
    return NULL;
  };
  
  lua_State* L = fluffyvm_get_executing_coroutine(F);
  
  test(NULL);

  /*pthread_t testThread;
  fluffyvm_start_thread(F, &testThread, NULL, test, NULL);
  
  pthread_t testThread2;
  fluffyvm_start_thread(F, &testThread2, NULL, test, NULL);
  
  pthread_t testThread3;
  fluffyvm_start_thread(F, &testThread3, NULL, test, NULL);
  
  pthread_t testThread4;
  fluffyvm_start_thread(F, &testThread4, NULL, test, NULL);
  
  pthread_t testThread5;
  fluffyvm_start_thread(F, &testThread5, NULL, test, NULL);
  
  pthread_t testThread6;
  fluffyvm_start_thread(F, &testThread6, NULL, test, NULL);
  
  pthread_join(testThread, NULL);
  pthread_join(testThread2, NULL);
  pthread_join(testThread3, NULL); 
  pthread_join(testThread4, NULL); 
  pthread_join(testThread5, NULL);
  pthread_join(testThread6, NULL); */
  
  fluffyvm_compat_lua54_lua_pop(L, fluffyvm_compat_lua54_lua_gettop(L));
  fluffyvm_clear_errmsg(F);
  fluffyvm_set_global(F, value_nil());

  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  collectAndPrintMemUsage("Before VM destruction but after test");
  
  fluffyvm_free(F);

  cannotCreateVm:
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  collectAndPrintMemUsage("After VM destruction");
  foxgc_api_free(heap);

  return 0;
}

int main() {
  int ret = 0;

  // First thing first load the damn bytecode
  // first
  FILE* bytecodeFile = fopen("./bytecode.json", "r");
  if (!bytecodeFile) {
    puts("File not found");
    goto do_return;
  }

  fseek(bytecodeFile, 0, SEEK_END);
  bytecodeRawLen = ftell(bytecodeFile);
  fseek(bytecodeFile, 0, SEEK_SET);
  
  bytecodeRaw = malloc(bytecodeRawLen);
  size_t result = fread(bytecodeRaw, 1, bytecodeRawLen, bytecodeFile);
  assert(result == bytecodeRawLen);
  
  fclose(bytecodeFile);

  ret = main2();
  free(bytecodeRaw);

  do_return:
  puts("Exiting :3");
  return ret;
}

