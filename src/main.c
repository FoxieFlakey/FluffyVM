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
  printMemUsage(tmp);
  free(tmp);
}

static void stdlib_print_stacktrace(struct fluffyvm* F, struct fluffyvm_coroutine* co) {
  const int tid = fluffyvm_get_thread_id(F);
  
  printf("[Thread %d] Stacktrace:\n", tid);
  coroutine_iterate_call_stack(F, co, true, ^bool (void* _arg) {
    struct fluffyvm_call_frame* frame = _arg;
    printf("[Thread %d] \t%s:%d: in %s\n", tid, frame->source, frame->line, frame->name);
    return true;
  });
}

static bool stdlib_print(struct fluffyvm* F, struct fluffyvm_call_state* callState, void* udata) {
  const int tid = fluffyvm_get_thread_id(F);

  for (int i = 0; i <= interpreter_get_top(F, callState); i++) {
    struct value string;
    interpreter_peek(F, callState, i, &string);
    printf("[Thread %d] Printer: %.*s\n", tid, (int) value_get_len(string), value_get_string(string));
  }

  stdlib_print_stacktrace(F, callState->owner);

  return true;
}

static bool stdlib_return_string(struct fluffyvm* F, struct fluffyvm_call_state* callState, void* udata) {
  foxgc_root_reference_t* tmpRootRef = NULL;
  
  {
    struct value string = value_new_string(F, "Returned from C function (Printed twice)", &tmpRootRef);
    interpreter_push(F, callState, string);
    foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), tmpRootRef);
  }

  {
    struct value string = value_new_string(F, "Returned from C function (Only printed once)", &tmpRootRef);
    interpreter_push(F, callState, string);
    foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), tmpRootRef);
  }

  return true;
}

static void registerCFunction(struct fluffyvm* F, struct value globalTable, const char* name, closure_cfunction_t cfunc) {
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
  
  fluffyvm_thread_routine_t test = ^void* (void* _) {
    const int tid = fluffyvm_get_thread_id(F);

    foxgc_root_reference_t* globalTableRootRef = NULL;
    struct value globalTable = value_new_table(F, 75, 32, &globalTableRootRef);

    registerCFunction(F, globalTable, "print", stdlib_print);
    registerCFunction(F, globalTable, "return_string", stdlib_return_string);

    foxgc_root_reference_t* bytecodeRootRef = NULL;
    struct fluffyvm_bytecode* bytecode = bytecode_loader_json_load(F, &bytecodeRootRef, bytecodeRaw, bytecodeRawLen);
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
   
    if (!coroutine_resume(F, co)) {
      puts("Error Occured!");
      struct value errMsg = co->thrownedError;
      printf("[Thread %d] Error: %.*s\n", tid, (int) value_get_len(errMsg), value_get_string(errMsg));
      stdlib_print_stacktrace(F, co);
      return NULL;
    }
    
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

  /* pthread_t testThread;
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
  

  collectAndPrintMemUsage("Before VM destruction but after test");
  
  fluffyvm_clear_errmsg(F);

  error:
  if (fluffyvm_is_errmsg_present(F)) {
    printMemUsage("At error");
    printf("Error: %s\n", value_get_string(fluffyvm_get_errmsg(F)));
  }

  fluffyvm_free(F);

  cannotCreateVm:
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

