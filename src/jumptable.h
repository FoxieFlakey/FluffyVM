#ifndef header_1664112102_3ab69f11_e6df_4219_bb2f_b0c06215ad1b_jumptable_h
#define header_1664112102_3ab69f11_e6df_4219_bb2f_b0c06215ad1b_jumptable_h

#include "config.h"
#include "opcodes.h"

#if !IS_ENABLED(CONFIG_USE_GOTO_POINTER)
# error "Included without CONFIG_USE_GOTO_POINTER enabled"
#endif

#define interpreter_dispatch(...) goto *jumpTable[instructionRegister.op];
#define interpreter_case(name) do_ ## name:
#define interpreter_break do { \
  interpreter_increment_ip(); \
  interpreter_fetch_instruction(); \
  goto *jumpTable[instructionRegister.op]; \
} while (0)

#define interpreter_break_no_increment_ip do { \
  interpreter_fetch_instruction(); \
  goto *jumpTable[instructionRegister.op]; \
} while (0)

static const void* const jumpTable[] = {
# define X(name, op, ...) \
  [op] = &&do_FLUFFYVM_ ## name,
  FLUFFYVM_OPCODES
  [FLUFFYVM_OPCODE_LAST] = &&do_FLUFFYVM_OPCODE_LAST
# undef X
};

#endif

