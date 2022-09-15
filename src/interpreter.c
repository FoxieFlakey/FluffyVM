#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "attributes.h"
#include "bytecode/bytecode.h"
#include "bytecode/prototype.h"
#include "constants.h"
#include "interpreter.h"
#include "call_state.h"
#include "performance.h"
#include "value.h"
#include "vm.h"
#include "opcodes.h"
#include "vm_types.h"

static math_operation_func mathOpLookup[FLUFFYVM_OPCODE_LAST] = {
  [FLUFFYVM_OPCODE_ADD] = value_add,
  [FLUFFYVM_OPCODE_SUB] = value_sub,
  [FLUFFYVM_OPCODE_MUL] = value_mul,
  [FLUFFYVM_OPCODE_DIV] = value_div,
  [FLUFFYVM_OPCODE_POW] = value_pow,
  [FLUFFYVM_OPCODE_MOD] = value_mod
};

int interpreter_exec(struct call_state* callstate) {
  uint32_t ip = 0;
  int flagRegister = 0;
  const struct prototype* proto = callstate->proto;
  bool jumped = false;

  struct value a;
  struct value b;
  struct vm* F = callstate->owner;

  // Interpreting loop
  for (; ip < proto->codeLen; ip++) {
    // When jumped `ip` not incremented
    // this is cleanest way
    if (jumped)
      ip--;
    jumped = false;

    struct instruction instructionRegister;
    if (opcode_decode_instruction(&instructionRegister, proto->code[ip]) < 0)
      goto execution_error;

    int condFlags = instructionRegister.cond & 0x0F;
    int condFlagsMask = (instructionRegister.cond & 0xF0) >> 4;

    // Condition didn't met skip current    
    if ((condFlags & FLUFFYVM_FLAG_EQUAL_MASK) != 
        ((flagRegister & condFlagsMask) & FLUFFYVM_FLAG_EQUAL_MASK) ||
        (condFlags & FLUFFYVM_FLAG_LESS_MASK) != 
        ((flagRegister & condFlagsMask) & FLUFFYVM_FLAG_LESS_MASK))
      continue;
    
    switch (instructionRegister.op) {
      case FLUFFYVM_OPCODE_NOP:
        break;
      case FLUFFYVM_OPCODE_LOAD_INTEGER:
        if (call_state_set_register(callstate, 
              instructionRegister.arg.u16_s32.a, 
              value_new_int(
                callstate->owner, 
                instructionRegister.arg.u16_s32.b
              ), true) < 0)
          goto illegal_instruction;
        break;
      case FLUFFYVM_OPCODE_MOV:
        if (call_state_move_register(callstate,
              instructionRegister.arg.u16x3.a, 
              instructionRegister.arg.u16x3.b) < 0)
          goto illegal_instruction;
        break;
      case FLUFFYVM_OPCODE_ADD:
      case FLUFFYVM_OPCODE_SUB:
      case FLUFFYVM_OPCODE_MUL:
      case FLUFFYVM_OPCODE_DIV:
        if (call_state_get_register(callstate, 
              &a, instructionRegister.arg.u16x3.b, true) < 0)
          goto illegal_instruction;
        
        if (call_state_get_register(callstate, 
              &b, instructionRegister.arg.u16x3.c, true) < 0)
          goto illegal_instruction;

        assert(mathOpLookup[instructionRegister.op]);
        if (mathOpLookup[instructionRegister.op](F, &a, a, b) < 0)
          goto illegal_instruction;

        if (call_state_set_register(callstate, 
              instructionRegister.arg.u16x3.a, 
              a, true) < 0)
          goto illegal_instruction;
        break;
      case FLUFFYVM_OPCODE_CMP:
        if (call_state_get_register(callstate,
              &a, instructionRegister.arg.u16x3.a, true) < 0)
          goto illegal_instruction;
        
        if (call_state_get_register(callstate, 
              &b, instructionRegister.arg.u16x3.b, true) < 0)
          goto illegal_instruction;
        
        flagRegister = 0x00;
        flagRegister |= value_is_equal(F, a, b) ? FLUFFYVM_FLAG_EQUAL_MASK : 0;
        flagRegister |= value_is_less(F, a, b) ? FLUFFYVM_FLAG_LESS_MASK : 0;
        break;
      case FLUFFYVM_OPCODE_GET_CONSTANT:
        if (bytecode_get_constant(callstate->proto->owner, F, &a, instructionRegister.arg.u16x3.b) < 0)
          goto illegal_instruction;
        if (call_state_set_register(callstate, instructionRegister.arg.u16x3.a, a, true) < 0)
          goto illegal_instruction;
        break;
      case FLUFFYVM_OPCODE_JMP_FORWARD:
        ip += instructionRegister.arg.u32;
        jumped = true;
        continue;
      case FLUFFYVM_OPCODE_JMP_BACKWARD:
        ip -= instructionRegister.arg.u32;
        jumped = true;
        continue;
      default:
        goto illegal_instruction;
    }
  }
  assert(ip == proto->codeLen);

  return 0;

  execution_error:
  return -EFAULT;

  illegal_instruction:
  return -EINVAL;
}


