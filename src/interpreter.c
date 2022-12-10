#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "attributes.h"
#include "bug.h"
#include "bytecode/bytecode.h"
#include "bytecode/prototype.h"
#include "vm_array_primitive.h"
#include "vm_limits.h"
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
  vm_instruction_pointer ip = 0;
  int flagRegister = 0;
  int res = 0;
  const struct prototype* proto = callstate->proto;

  struct value a, b, c;
  struct vm* F = callstate->owner;

# define load(result, reg) do { \
  if (call_state_get_register(callstate, \
        result, instructionRegister.arg.u16x3.reg, true) < 0) { \
    res = -EINVAL; \
    goto illegal_instruction; \
  } \
} while (0)

# define store(reg, val) do { \
  if (call_state_set_register(callstate, \
        instructionRegister.arg.u16x3.reg, val, true) < 0) { \
    res = -EINVAL; \
    goto illegal_instruction; \
  } \
} while (0)

# define storeA() store(a, a)
# define storeB() store(b, b)
# define storeC() store(c, c)

# define loadA() load(&a, a)
# define loadB() load(&b, b)
# define loadC() load(&c, c)

//#undef CONFIG_USE_GOTO_POINTER
# if IS_ENABLED(CONFIG_USE_GOTO_POINTER)
#   include "jumptable.h"
  
  // For supressing unused goto labels warnings
  if (0)
    goto break_current_cycle;
# else
#   define interpreter_dispatch switch
#   define interpreter_case(name) case name:
#   define interpreter_break do { \
      interpreter_increment_ip(); \
      interpreter_fetch_instruction(); \
      goto break_current_cycle; \
    } while (0)
#   define interpreter_break_no_increment_ip do { \
      interpreter_fetch_instruction(); \
      goto break_current_cycle; \
    } while (0)
# endif
  
    // Interpreting loop
  for (;;) {
    struct instruction instructionRegister;
#   define interpreter_increment_ip() do { \
      ip++; \
    } while (0)
#   define interpreter_fetch_instruction() do { \
      if (ip >= proto->preDecoded.length) \
        goto exit_func; \
       \
      instructionRegister = proto->preDecoded.data[ip]; \
      /* assert(opcode_decode_instruction(&instructionRegister, proto->code[ip]) >= 0); */ \
      int condFlags = instructionRegister.cond & 0x0F; \
      int condFlagsMask = (instructionRegister.cond & 0xF0) >> 4; \
       \
      /* Condition didn't met skip current */ \
      if ((condFlags & OP_COND_EQ_MASK) != \
          ((flagRegister & condFlagsMask) & OP_COND_EQ_MASK) || \
          (condFlags & OP_COND_LT_MASK) !=  \
          ((flagRegister & condFlagsMask) & OP_COND_LT_MASK)) { \
        interpreter_increment_ip(); \
        goto skip_to_next_cycle; \
      } \
    } while (0)

skip_to_next_cycle:
    interpreter_fetch_instruction();

break_current_cycle:
    interpreter_dispatch(instructionRegister.op) {
      interpreter_case(FLUFFYVM_OPCODE_NOP)
        interpreter_break;
      interpreter_case(FLUFFYVM_OPCODE_LOAD_INTEGER)
        store(a, value_new_int(callstate->owner, instructionRegister.arg.u16_s32.b));
        interpreter_break;
      interpreter_case(FLUFFYVM_OPCODE_MOV)
        if (call_state_move_register(callstate,
              instructionRegister.arg.u16x3.a, 
              instructionRegister.arg.u16x3.b) < 0) {
          res = -EINVAL;
          goto illegal_instruction;
        }
        interpreter_break;
      interpreter_case(FLUFFYVM_OPCODE_ADD)
      interpreter_case(FLUFFYVM_OPCODE_SUB)
      interpreter_case(FLUFFYVM_OPCODE_MUL)
      interpreter_case(FLUFFYVM_OPCODE_DIV)
      interpreter_case(FLUFFYVM_OPCODE_POW)
      interpreter_case(FLUFFYVM_OPCODE_MOD)
        load(&a, b);
        load(&b, c);
        
        if (mathOpLookup[instructionRegister.op](F, &a, a, b) < 0) {
          res = -EINVAL;
          goto illegal_instruction;
        }

        storeA();
        interpreter_break;
      interpreter_case(FLUFFYVM_OPCODE_CMP)
        loadA();
        loadB();
        
        flagRegister = 0x00;
        flagRegister |= value_is_equal(F, a, b) ? OP_COND_EQ_MASK : 0;
        flagRegister |= value_is_less(F, a, b) ? OP_COND_LT_MASK : 0;
        interpreter_break;
      interpreter_case(FLUFFYVM_OPCODE_NEW_ARRAY)
        store(a, value_new_primitive_array(F, instructionRegister.arg.u16_u32.b));
        interpreter_break;
      interpreter_case(FLUFFYVM_OPCODE_LOAD_CONSTANT)
        if (bytecode_get_constant(callstate->proto->owner, F, &a, instructionRegister.arg.u16x3.b) < 0)
          goto illegal_instruction;
        storeA();
        interpreter_break;
      interpreter_case(FLUFFYVM_OPCODE_JMP_FORWARD)
        ip += instructionRegister.arg.u32;
        interpreter_break_no_increment_ip;
      interpreter_case(FLUFFYVM_OPCODE_JMP_BACKWARD)
        ip -= instructionRegister.arg.u32;
        interpreter_break_no_increment_ip;
      interpreter_case(FLUFFYVM_OPCODE_GET_ARRAY) 
      interpreter_case(FLUFFYVM_OPCODE_SET_ARRAY)
        loadA();
        loadB();
        
        if (a.type != VALUE_PRIMITIVE_ARRAY ||
            b.type != VALUE_INTEGER ||
            b.data.integer < 0 || b.data.integer > VM_INT_MAX) {
          res = -EFAULT;
          goto exec_error;
        }
        
        if (instructionRegister.op == FLUFFYVM_OPCODE_SET_ARRAY) {
          loadC();
          
          if (value_is_byref(F, c) == true) {
            res = -EFAULT;
            goto exec_error;
          }
          
          if ((res = array_primitive_set(F, a.data.primitiveArray, b.data.integer, c)) < 0)
            goto exec_error;
        } else {
          if ((res = array_primitive_get(F, a.data.primitiveArray, b.data.integer, &c)) < 0)
            goto exec_error;
          
          storeC();
        }
        interpreter_break;
      interpreter_case(FLUFFYVM_OPCODE_RET)
        goto exit_func;
      interpreter_case(FLUFFYVM_OPCODE_LAST)
        goto illegal_instruction;
      
      interpreter_case(FLUFFYVM_OPCODE_IMPLDEP1) 
      interpreter_case(FLUFFYVM_OPCODE_IMPLDEP2) 
      interpreter_case(FLUFFYVM_OPCODE_IMPLDEP3) 
      interpreter_case(FLUFFYVM_OPCODE_IMPLDEP4)
      interpreter_case(FLUFFYVM_OPCODE_LOAD_PROTOTYPE) 
        BUG();
    }
  }

// TODO: possibly add proper exception throw once exception system is implemented
exec_error:
illegal_instruction:
exit_func:
  return res;
}


