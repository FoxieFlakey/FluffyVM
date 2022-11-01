#include <stdlib.h>
#include <stdio.h>
#include <Block.h>
#include <assert.h>

#include <FluffyGC/v1.h>
#include <time.h>

#include "bytecode/bytecode.h"
#include "bytecode/prototype.h"
#include "call_state.h"
#include "coroutine.h"
#include "fiber.h"
#include "interpreter.h"
#include "opcodes.h"
#include "value.h"
#include "vm.h"
#include "vm_types.h"

#define KiB * 1024
#define MiB * 1024 KiB
#define GiB * 1024 MiB
#define TiB * 1024 GiB

int main2() {
  fluffygc_state* heap = NULL; /*fluffygc_v1_new(
      8 MiB, 
      16 MiB, 
      64 KiB, 
      100, 
      0.45f,
      65536);*/
  struct vm* F = vm_new(heap);
  struct bytecode* bytecode = NULL;
  struct call_state* callstate = call_state_new(F, bytecode->mainPrototype);
  
  //////////////////////
  clock_t start = clock();
  int ret = interpreter_exec(callstate);
  double time = ((double) (clock() - start)) / CLOCKS_PER_SEC * 1000;
  printf("Time took: %.2lf ms (ret: %d)\n", time, ret);
  //////////////////////

  struct value tmp;
  call_state_get_register(callstate, &tmp, 0, false);
  printf("Result: %" PRINT_VM_INT_d "\n", tmp.data.integer);
  
  call_state_get_register(callstate, &tmp, 1, false);
  printf("Constant 1: %" PRINT_VM_INT_d "\n", tmp.data.integer);
  
  call_state_get_register(callstate, &tmp, 2, false);
  printf("Constant 2: %" PRINT_VM_NUMBER "\n", tmp.data.number);
  
  call_state_get_register(callstate, &tmp, 3, false);
  printf("Addition 3: %" PRINT_VM_NUMBER "\n", tmp.data.number);


  call_state_free(callstate);
  bytecode_free(bytecode);
  
  vm_free(F);
  fluffygc_v1_free(heap);
  return EXIT_SUCCESS;
}

