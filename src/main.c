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

ATTRIBUTE((format(printf, 1, 2)))
static void collectAndPrintMemUsage(const char* fmt, ...) {
  char* tmp;
  
  va_list args;
  va_start(args, fmt);
  util_vasprintf(&tmp, fmt, args);
  va_end(args);

  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  printMemUsage(tmp);
  free(tmp);
}

static bool stdlib_print(struct fluffyvm* F, struct fluffyvm_call_state* callState, void* udata) {
  foxgc_root_reference_t* tmpRootRef = NULL;
  struct value string;
  coroutine_yield(F);
  coroutine_yield(F);
  while (interpreter_pop(F, callState, &string, &tmpRootRef)) {
    printf("[Thread %d] Printer: %.*s\n", fluffyvm_get_thread_id(F), (int) value_get_len(string), value_get_string(string));
    foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), tmpRootRef);
  }

  return true;
}

int main2() {
  heap = foxgc_api_new(1 * MB, 4 * MB, 16 * MB,
                                   3, 3, 

                                 8 * KB, 1 * MB,

                                 32 * KB);
  collectAndPrintMemUsage("Before VM creation");

  struct fluffyvm* F = fluffyvm_new(heap);
  if (!F) {
    printf("FATAL: can't create VM\n");
    goto cannotCreateVm;
  }

  collectAndPrintMemUsage("After VM creation but before test");
  
  // Bootloader  
  const char* bootloader = data_bootloader; 
  
  fluffyvm_thread_routine_t test = ^void* (void* _) {
    const int tid = fluffyvm_get_thread_id(F);

    foxgc_root_reference_t* globalTableRootRef = NULL;
    struct value globalTable = value_new_table(F, 75, 32, &globalTableRootRef);

    {
      foxgc_root_reference_t* printRootRef = NULL;
      foxgc_root_reference_t* printStringRootRef = NULL;
      struct fluffyvm_closure* printFunc = closure_from_cfunction(F, &printRootRef, stdlib_print, NULL, NULL, globalTable);
      if (!printFunc)
        goto error;

      struct value printVal = value_new_closure(F, printFunc);
      struct value printString = value_new_string(F, "print", &printStringRootRef);
      if (printString.type == FLUFFYVM_TVALUE_NOT_PRESENT)
        goto error;

      value_table_set(F, globalTable, printString, printVal);

      foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), printRootRef);
      foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), printStringRootRef);
    }

    foxgc_root_reference_t* bytecodeRootRef = NULL;
    struct fluffyvm_bytecode* bytecode = bytecode_loader_json_load(F, &bytecodeRootRef, bootloader, data_bootloader_get_len() - 1);
    if (!bytecode)
      goto error; 
    
    foxgc_root_reference_t* closureRootRef = NULL;
    struct fluffyvm_closure* closure = closure_new(F, &closureRootRef, bytecode->mainPrototype, globalTable);
    if (!closure)
      goto error;
    foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), bytecodeRootRef);
    foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), globalTableRootRef);

    foxgc_root_reference_t* coroutineRootRef = NULL;
    struct fluffyvm_coroutine* co = coroutine_new(F, &coroutineRootRef, closure);
    foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), closureRootRef);
    if (!co)
      goto error;
    
    collectAndPrintMemUsage("[Thread %d] Coroutine created", tid);
   
    
    if (!coroutine_resume(F, co))
      goto error;
    if (!coroutine_resume(F, co))
      goto error;
    if (!coroutine_resume(F, co))
      goto error;

    collectAndPrintMemUsage("[Thread %d] Bytecode executed", tid);
    foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), coroutineRootRef);
    
    collectAndPrintMemUsage("[Thread %d] After test", tid);
    return NULL;

    error:;
    if (fluffyvm_is_errmsg_present(F))
      collectAndPrintMemUsage("[Thread %d] Error: %.*s\n", tid, (int) value_get_len(fluffyvm_get_errmsg(F)), value_get_string(fluffyvm_get_errmsg(F)));
    return NULL;
  };
  
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
  pthread_join(testThread6, NULL);*/
  

  collectAndPrintMemUsage("Before VM destruction but after test");
  
  fluffyvm_clear_errmsg(F);

  error:
  if (fluffyvm_is_errmsg_present(F)) {
    printMemUsage("At error");
    printf("Error: %s\n", value_get_string(fluffyvm_get_errmsg(F)));
  }

  fluffyvm_free(F);

  cannotCreateVm:
  collectAndPrintMemUsage("After VM destruction");
  foxgc_api_free(heap);

  return 0;
}

int main() {
  int ret = main2();

  puts("Exiting :3");
  return ret;
}

