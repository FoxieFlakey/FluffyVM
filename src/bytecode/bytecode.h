#ifndef header_1662272744_52d075c5_7f9b_446c_9a2a_133655f3312d_bytecode_h
#define header_1662272744_52d075c5_7f9b_446c_9a2a_133655f3312d_bytecode_h

#include <stddef.h>
#include <stdint.h>

#include "vec.h"
#include "vm_types.h"
#include "buffer.h"

struct vm;
struct value;
struct prototype;

enum constant_type {
  BYTECODE_CONSTANT_INTEGER,
  BYTECODE_CONSTANT_NUMBER,
  BYTECODE_CONSTANT_STRING
};

struct constant {
  enum constant_type type;
  union {
    vm_int integer;
    vm_number number;
    const char* string;
  } data;
};

struct bytecode {
  vec_t(struct constant) constants;
  struct prototype* mainPrototype;
};

struct bytecode* bytecode_new();
void bytecode_free(struct bytecode* self);

// bytecode_add_constant_* return constant index
// On error return negative errno
// Errors:
// -ENOMEM: Not enough memory
// -ENOSPC: Number of constants exceeded VM_LIMIT_MAX_CONSTANT
int bytecode_add_constant_generic(struct bytecode* self, struct constant constant);
int bytecode_add_constant_int(struct bytecode* self, vm_int integer);
int bytecode_add_constant_number(struct bytecode* self, vm_number number);
int bytecode_add_constant_string(struct bytecode* self, const char* integer);

/* Errors:
 * -ERANGE: Invalid constant index
 * -ENOMEM: Out of memory
 */
int bytecode_get_constant(struct bytecode* self, struct vm* vm, struct value* result, uint32_t index);

#endif

