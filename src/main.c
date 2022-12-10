#include <stdlib.h>
#include <stdio.h>
#include <Block.h>
#include <assert.h>
#include <string.h>

#include <FluffyGC/v1.h>
#include <time.h>

#include "bug.h"
#include "bytecode/bytecode.h"
#include "bytecode/prototype.h"
#include "vm.h"
#include "bytecode/protobuf_deserializer.h"
#include "call_state.h"
#include "coroutine.h"
#include "fiber.h"
#include "interpreter.h"
#include "opcodes.h"
#include "value.h"
#include "vm.h"
#include "vm_string.h"
#include "vm_types.h"

#define KiB * 1024
#define MiB * 1024 KiB
#define GiB * 1024 MiB
#define TiB * 1024 GiB

int main2(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: %s <bytecode to run>\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char* bytecodePath = argv[1];
  void* bytecodeRaw = NULL;
  size_t bytecodeSize = 0;
  FILE* fp = fopen(bytecodePath, "rb");
  if (!fp)
    abort();
  fseek(fp, 0, SEEK_END);
  bytecodeSize = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  bytecodeRaw = malloc(bytecodeSize);
  assert(fread(bytecodeRaw, 1, bytecodeSize, fp) == bytecodeSize);
  fclose(fp);
  
  int res = 0;
  struct bytecode* bytecode = NULL;
  int version = 0;
  if ((res = bytecode_deserializer_protobuf(&bytecode, &version, bytecodeRaw, bytecodeSize)) < 0) {
    printf("Fail deserializing bytecode: %s\n", strerror(res));
    res = EXIT_FAILURE;
    goto deserialize_error;
  }
  
  printf("Bytecode version: %d\n", version);
  BUG_ON(bytecode == NULL);
  
  // Actually interpreting
  fluffygc_state* heap = fluffygc_v1_new(
      8 MiB, 
      16 MiB, 
      64 KiB, 
      100, 
      0.45f,
      65536);
  if (!heap) {
    res = EXIT_FAILURE;
    goto heap_alloc_error;
  }
  fluffygc_v1_attach_thread(heap);
  
  struct vm* F = vm_new(heap);
  if (!F) {
    res = EXIT_FAILURE;
    goto vm_alloc_error;
  }
  
  struct string_gcobject* obj = string_from_cstring(F, "Hi");
  const char* strPtr = NULL;
  printf("String: %s\n", (strPtr = string_get_critical(F, obj)));
  string_release_critical(F, obj, strPtr);
  fluffygc_v1_delete_local_ref(heap, cast_to_gcobj(obj));
  
  struct call_state* callstate = call_state_new(F, bytecode->mainPrototype);
  call_state_set_register(callstate, 0, value_new_int(F, 1 /* 1000000 */ ), false);
  
  //////////////////////
  clock_t start = clock();
  int ret = interpreter_exec(callstate);
  double time = ((double) (clock() - start)) / CLOCKS_PER_SEC * 1000;
  printf("Time took: %.2lf ms (ret: %d)\n", time, ret);
  //////////////////////

  struct value tmp;
  call_state_get_register(callstate, &tmp, 0, false);
  switch (tmp.type) {
    case VALUE_INTEGER:
      printf("Result: %" PRINT_VM_INT_d "\n", tmp.data.integer);
      break;
    case VALUE_NONE:
      printf("Result: None\n");
      break;
    case VALUE_STRING: {
      const char* strPtr = NULL;
      printf("Result: %s\n", (strPtr = string_get_critical(F, tmp.data.string)));
      string_release_critical(F, tmp.data.string, strPtr);
      break;
    }
  }   

  call_state_free(callstate);

early_exit:
  puts("Calling full GC!");
  fluffygc_v1_trigger_full_gc(heap);
  bytecode_free(bytecode);

  vm_free(F);
  fluffygc_v1_detach_thread(heap);
vm_alloc_error:
  fluffygc_v1_free(heap);
heap_alloc_error:
deserialize_error:
  free(bytecodeRaw);
  return EXIT_SUCCESS;
}

