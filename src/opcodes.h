#ifndef header_1662260844_a4351b30_63ec_40e1_b9b4_1edf128fdae4_opcodes_h
#define header_1662260844_a4351b30_63ec_40e1_b9b4_1edf128fdae4_opcodes_h

#include <stdint.h>
#include <errno.h>

#include "performance.h"
#include "vm_types.h"

struct call_state;
struct vm;

/*
 * X(name, opcode, nameInString, fieldsUsed, layout)
 *
 * fieldsUsed is number of 16-bit fields used
 * irrespective to the layout
 */

#define FLUFFYVM_OPCODES \
  X(OPCODE_NOP, 0x00, "nop", 0, OP_LAYOUT_u16x3) \
  X(OPCODE_MOV, 0x01, "mov", 2, OP_LAYOUT_u16x3) \
  /*X(OPCODE_TABLE_GET, 0x02, "table_get", 3, OP_LAYOUT_u16x3)*/ \
  /*X(OPCODE_TABLE_SET, 0x03, "table_set", 3, OP_LAYOUT_u16x3)*/ \
  X(OPCODE_ADD, 0x04, "add", 3, OP_LAYOUT_u16x3) \
  X(OPCODE_SUB, 0x05, "sub", 3, OP_LAYOUT_u16x3) \
  X(OPCODE_MUL, 0x06, "mul", 3, OP_LAYOUT_u16x3) \
  X(OPCODE_DIV, 0x07, "div", 3, OP_LAYOUT_u16x3) \
  X(OPCODE_MOD, 0x08, "mod", 3, OP_LAYOUT_u16x3) \
  X(OPCODE_POW, 0x09, "pow", 3, OP_LAYOUT_u16x3) \
  X(OPCODE_JMP_FORWARD, 0x0A, "jmp_forward", 2, OP_LAYOUT_u32) \
  X(OPCODE_JMP_BACKWARD, 0x0B, "jmp_backward", 2, OP_LAYOUT_u32) \
  X(OPCODE_CMP, 0x0C, "cmp", 2, OP_LAYOUT_u16x3) \
  X(OPCODE_GET_CONSTANT, 0x0D, "get_constant", 3, OP_LAYOUT_u16_u32) \
  X(OPCODE_LOAD_INTEGER, 0x0E, "load_integer", 3, OP_LAYOUT_u16_s32)

/*
 * Instruction format
 * Offset counted from MSB
 * Byte 0   : Opcode
 * Byte 1   : Condition
 * Byte 2-3 : Field A
 * Byte 4-5 : Field B
 * Byte 6-7 : Field C
 */

enum fluffyvm_opcode {
# define X(name, op, ...) FLUFFYVM_ ## name = op,
  FLUFFYVM_OPCODES
# undef X
  FLUFFYVM_OPCODE_LAST
};

struct instruction {
  enum fluffyvm_opcode op;
  uint8_t cond;

  union {
    struct {
      uint16_t a;
      uint16_t b;
      uint16_t c;
    } u16x3;
    
    // Primarily for loading integers
    struct {
      uint16_t a;
      uint32_t b;
    } u16_u32;
    
    // Primarily for loading integers
    struct {
      uint16_t a;
      int32_t b;
    } u16_s32;

    uint32_t u32;
  } arg;
};

/*
 * Returns NULL on error
 */
const char* opcode_tostring(enum fluffyvm_opcode op);

/*
 * Errors:
 * -EINVAL: Invalid opcode
 */
int opcode_get_field_count(enum fluffyvm_opcode op);

/*
 * Errors:
 * -EINVAL: Invalid opcode
 */
static inline int opcode_get_layout(enum fluffyvm_opcode op);

enum instruction_layout {
  OP_LAYOUT_u16x3,
  OP_LAYOUT_u16_u32,
  OP_LAYOUT_u16_s32,

  // Exactly the same OP_LAYOUT_u16_u32
  // but without the u16
  OP_LAYOUT_u32
};

// Passing NULL to result will just check
// instruction for validity
//
// Error:
// -EINVAL: Invalid instruction
int opcode_decode_instruction(struct instruction* result, vm_instruction instruction);

////////////////////////////////
/// Inlined stuff below here ///
////////////////////////////////

extern enum instruction_layout opcodes_layoutLookup[];

static inline int opcode_get_layout(enum fluffyvm_opcode op) {
  if (op >= FLUFFYVM_OPCODE_LAST || op < 0)
    return -EINVAL;

  return opcodes_layoutLookup[op];
}

#endif


