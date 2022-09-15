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
  fluffygc_state* heap = fluffygc_v1_new(
      8 MiB, 
      16 MiB, 
      64 KiB, 
      100, 
      0.45f,
      65536);
  struct vm* F = vm_new(heap);
  struct bytecode* bytecode = bytecode_new();
  struct prototype* prototypes[] = {
    prototype_new()
  };

  /*
  local COND_AL = 0x00  -- Always     (0b0000'0000)

  local COND_EQ = 0x11  -- Equal      (0b0001'0001)
  local COND_LT = 0x32  -- Less than  (0b0011'0010)
  local COND_NE = 0x10  -- Not equal  (0b0001'0000)
  local COND_GT = 0x30  -- Greater    (0b0011'0000)
  local COND_GE = COND_GT | COND_EQ -- 0x31
  local COND_LE = COND_LT | COND_EQ -- 0x33
  */

  vm_instruction prog_factorial[] = {
    0x0E0000020000000F, // R(2) = 0x0F (16)
                        // n
    0x0E00000300000001, // R(3) = 0x01 (1)
                        // constant one
    
    0x0E00000400000000, // R(4) = 0x00 (0)
                        // current loop count
    0x0E0000050000000A,//F4240, // R(5) = 0x00 (1000000)
                        // loop count
    
    // Just off by one errors
    0x0400000200020003, // R(2) = R(2) + R(3)
    
    0x0C00000400050000, // Compare R(4) with R(5)
    0x0A110000000A0000, // IP += 10 if equal-than
      0x0E00000000000001, // R(0) = 0x00 (0)
                          // current_product
      0x0E00000100000001, // R(1) = 0x01 (1)
                          // counter
    
      0x0C00000100020000, // Compare R(1) with R(2)
      0x0A30000000040000, // IP += 4 if greater-than
        0x0600000000000001, // R(0) = R(0) * R(1)
        0x0400000100010003, // R(1) = R(1) + R(3)
      0x0B00000000040000, // IP -= 4

      0x0400000400040003, // R(4) = R(4) + R(3)
    0x0B000000000A0000, // IP -= 10
    0x0000000000000000, // No-op
    
    0x0D00000100010000, // R(1) = Const[1]
    0x0D00000200020000, // R(2) = Const[2]
    0x0400000300010002 // R(3) = R(1) + R(2)
  };

  struct constant prog_factorial_const[] = {
    {
      .type = BYTECODE_CONSTANT_INTEGER,
      .data.integer = 0
    },
    {
      .type = BYTECODE_CONSTANT_INTEGER,
      .data.integer = 9028
    },
    {
      .type = BYTECODE_CONSTANT_NUMBER,
      .data.number = 3.14
    }
  };

  vm_instruction* code = prog_factorial;
  struct constant* constants = prog_factorial_const;

  size_t codeCount = sizeof(prog_factorial) / sizeof(*prog_factorial);
  size_t constantsCount = sizeof(prog_factorial_const) / sizeof(*prog_factorial_const);

  prototype_set_code(prototypes[0], codeCount, code);
  bytecode_set_prototypes(bytecode, sizeof(prototypes) / sizeof(struct prototype*), prototypes);
  bytecode_set_constants(bytecode, constantsCount, constants);

  struct call_state* callstate = call_state_new(F, prototypes[0]);
  
  //////////////////////
  clock_t start = clock();
  interpreter_exec(callstate);
  double time = ((double) (clock() - start)) / CLOCKS_PER_SEC * 1000;
  printf("Time took: %.2lf ms\n", time);
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

