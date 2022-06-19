#include <assert.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <Block.h>
#include <time.h>

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
#include "string_cache.h"

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

static int stdlib_print(lua_State* L) {
  const int tid = fluffyvm_get_thread_id(L->owner);

  for (int i = 0; i < fluffyvm_compat_lua54_lua_gettop(L); i++)
    printf("\t%s", fluffyvm_compat_lua54_lua_tostring(L, i + 1));

  /*
  static atomic_bool hasDump = false;
  if (atomic_exchange(&hasDump, true) == false) {
    clock_t startClock = clock();

    puts("Dumping...");
    const char* errMsg;
    if (!foxgc_api_heap_dump(F->heap, "heapDump.sqlite3", &errMsg))
      printf("[WARNING] Cannot dump heap: %s\n", errMsg);
    puts("Done...");

    clock_t timeSpent = clock() - startClock;
    printf("Dumping took %lf seconds\n", ((double) timeSpent) / CLOCKS_PER_SEC);
    free((char*) errMsg);
  }
  */

  stdlib_print_stacktrace(L->owner, L->currentCallState->owner);
  return 0;
}

static int stdlib_error(lua_State* L) {
  fluffyvm_compat_lua54_lua_error(L);
  return 0;
}

static int stdlib_getCPUTime(lua_State* L) {
  fluffyvm_compat_lua54_lua_pushnumber(L, ((lua_Number) clock()) / CLOCKS_PER_SEC * 1000);
  return 1;
}

#define UNIQUE_KEY(name) static uintptr_t name = (uintptr_t) &name

UNIQUE_KEY(testArrayTypeKey);

static void test(struct fluffyvm* F) {
  foxgc_root_reference_t* rootRef = NULL;
  foxgc_root_reference_t* rootRef2 = NULL;
  foxgc_root_reference_t* rootRef3 = NULL;
  foxgc_object_t* tmp = NULL;

  foxgc_object_t* obj = foxgc_api_new_array(F->heap, fluffyvm_get_owner_key(), testArrayTypeKey, NULL, fluffyvm_get_root(F), &rootRef, 0, ^void (foxgc_object_t* _) {
    puts("[Weak Ref Test] Collected by weak ref");
  });

  foxgc_reference_t* ref = foxgc_api_new_weak_reference(F->heap, fluffyvm_get_root(F), &rootRef2, obj);
  
  tmp = foxgc_api_reference_get(ref, fluffyvm_get_root(F), &rootRef3);
  assert(tmp != NULL);
  foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), rootRef3);

  foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), rootRef);  
  foxgc_api_do_full_gc(F->heap);

  tmp = foxgc_api_reference_get(ref, fluffyvm_get_root(F), &rootRef3);
  assert(tmp == NULL);

  foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), rootRef2);
}

/* Generation sizes guide
 * 1 : 2 : 8
 */
int main2() {
  heap = foxgc_api_new(32 * MB, 64 * MB, 128 * MB,
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
    
  lua_State* L = fluffyvm_get_executing_coroutine(F);
  fluffyvm_compat_lua54_lua_register(L, "print", stdlib_print);
  fluffyvm_compat_lua54_lua_register(L, "error", stdlib_error);
  fluffyvm_compat_lua54_lua_register(L, "getCPUTime", stdlib_getCPUTime);

  /*
  struct value globalTable = F->globalTable;
  struct hashtable* table = foxgc_api_object_get_data(globalTable.data.table); 
  struct value key = hashtable_next(F, table, value_not_present());
  for (;key.type != FLUFFYVM_TVALUE_NOT_PRESENT;
        value_copy(&key, hashtable_next(F, table, key))) {
    printf("Key: %s\n", (char*) foxgc_api_object_get_data(key.data.str->str));
  }
  */

  test(F);

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
  
  test(NULL);

  /*
  pthread_t testThread;
  fluffyvm_start_thread(F, &testThread, NULL, Block_copy(test), NULL);
  
  pthread_t testThread2;
  fluffyvm_start_thread(F, &testThread2, NULL, Block_copy(test), NULL);
  
  pthread_t testThread3;
  fluffyvm_start_thread(F, &testThread3, NULL, Block_copy(test), NULL);
  
  pthread_t testThread4;
  fluffyvm_start_thread(F, &testThread4, NULL, Block_copy(test), NULL);
  
  pthread_t testThread5;
  fluffyvm_start_thread(F, &testThread5, NULL, Block_copy(test), NULL);
  
  pthread_t testThread6;
  fluffyvm_start_thread(F, &testThread6, NULL, Block_copy(test), NULL);
  
  pthread_join(testThread, NULL);
  pthread_join(testThread2, NULL);
  pthread_join(testThread3, NULL); 
  pthread_join(testThread4, NULL); 
  pthread_join(testThread5, NULL);
  pthread_join(testThread6, NULL);
  */
  
  fluffyvm_compat_lua54_lua_pop(L, fluffyvm_compat_lua54_lua_gettop(L));
  fluffyvm_clear_errmsg(F);
  fluffyvm_set_global(F, value_nil());

  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  string_cache_poll(F, F->stringCache, NULL);
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

