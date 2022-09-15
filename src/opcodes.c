#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <stdint.h>

#include "call_state.h"
#include "constants.h"
#include "opcodes.h"
#include "value.h"

static const char* nameLookup[] = {
# define X(_0, op, name, ...) [op] = name,
  FLUFFYVM_OPCODES
# undef X
};

int fieldCountLookup[] = {
# define X(_0, op, _1, fieldCount, ...) [op] = fieldCount,
  FLUFFYVM_OPCODES
# undef X
};

enum instruction_layout opcodes_layoutLookup[] = {
# define X(_0, op, _1, _2, layout, ...) [op] = layout,
  FLUFFYVM_OPCODES
# undef X
};

const char* opcode_tostring(enum fluffyvm_opcode op) {
  if (op >= FLUFFYVM_OPCODE_LAST || op < 0)
    return NULL;

  return nameLookup[op];
}

int opcode_get_field_count(enum fluffyvm_opcode op) {
  if (op >= FLUFFYVM_OPCODE_LAST || op < 0)
    return -EINVAL;

  return fieldCountLookup[op];
}

int opcode_decode_instruction(struct instruction* result, vm_instruction instruction) {
  struct instruction temp = {
    .op          = (instruction >> 56) & 0xFF,
    .cond        = (instruction >> 48) & 0xFF,
    .arg.u16x3.a = (instruction >> 32) & 0xFFFF,
    .arg.u16x3.b = (instruction >> 16) & 0xFFFF,
    .arg.u16x3.c =  instruction        & 0xFFFF
  };

  if (temp.op >= FLUFFYVM_OPCODE_LAST)
    return -EINVAL;

  int layout = opcode_get_layout(temp.op);
  if (layout < 0)
    return -EINVAL;
  
  // Now converting to layout
  uint16_t a_u16 = temp.arg.u16x3.a;
  uint32_t b_u32 = (temp.arg.u16x3.b << 16) | 
                    temp.arg.u16x3.c;
  switch (layout) {
    case OP_LAYOUT_u16_u32:
      temp.arg.u16_u32.a = a_u16;
      temp.arg.u16_u32.b = b_u32;
      break;
    case OP_LAYOUT_u16_s32:
      temp.arg.u16_s32.a = a_u16;
      temp.arg.u16_s32.b = (int32_t) b_u32;
      break;
    case OP_LAYOUT_u32:
      temp.arg.u32 = (temp.arg.u16x3.a << 16) | 
                      temp.arg.u16x3.b;
      break;
    case OP_LAYOUT_u16x3:
      break;
    default:
      abort();
  }

  if (result)
    *result = temp;
  return 0;
}

