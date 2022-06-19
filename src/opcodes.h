#ifndef header_1652512507_opcodes_h
#define header_1652512507_opcodes_h

/*
 * X(name, opcode, nameInString, fieldsUsed)
 */
#define FLUFFYVM_OPCODES \
  X(OPCODE_NOP, 0x00, "nop", 0) \
  X(OPCODE_MOV, 0x01, "mov", 2) \
  X(OPCODE_TABLE_GET, 0x02, "table_get", 3) \
  X(OPCODE_TABLE_SET, 0x03, "table_set", 3) \
  X(OPCODE_CALL, 0x04, "call", 3) \
  X(OPCODE_STACK_PUSH, 0x05, "stack_push", 1) \
  X(OPCODE_STACK_POP, 0x06, "stack_pop", 1) \
  X(OPCODE_GET_CONSTANT, 0x07, "get_constant", 2) \
  X(OPCODE_RETURN, 0x08, "ret", 2) \
  X(OPCODE_EXTRA, 0x09, "extra", 3) \
  X(OPCODE_STACK_GETTOP, 0x0A, "stack_gettop", 1) \
  X(OPCODE_LOAD_PROTOTYPE, 0x0B, "load_prototype", 2) \
  X(OPCODE_ADD, 0x0C, "add", 3) \
  X(OPCODE_SUB, 0x0D, "sub", 3) \
  X(OPCODE_MUL, 0x0E, "mul", 3) \
  X(OPCODE_DIV, 0x0F, "div", 3) \
  X(OPCODE_MOD, 0x10, "mod", 3) \
  X(OPCODE_POW, 0x11, "pow", 3) \
  X(OPCODE_JMP_FORWARD, 0x12, "jmp_forward", 1) \
  X(OPCODE_JMP_BACKWARD, 0x13, "jmp_backward", 1) \
  X(OPCODE_CMP, 0x14, "cmp", 2)

typedef enum {
# define X(name, op, ...) FLUFFYVM_ ## name = op,
  FLUFFYVM_OPCODES
# undef X
  FLUFFYVM_OPCODE_LAST
} fluffyvm_opcode_t;

#endif

