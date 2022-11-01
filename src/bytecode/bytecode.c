#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "bytecode.h"
#include "prototype.h"
#include "value.h"
#include "vm_limits.h"
#include "vm_types.h"
#include "vec.h"
#include "vm_limits.h"

struct bytecode* bytecode_new() {
  struct bytecode* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  self->mainPrototype = NULL;
  vec_init(&self->constants);
  return self;
}

void bytecode_free(struct bytecode* self) {
  if (!self)
    return;

  prototype_free(self->mainPrototype);
  
  int i;
  struct constant constant;
  vec_foreach(&self->constants, constant, i)
    if (constant.type == BYTECODE_CONSTANT_STRING)
      free((char*) constant.data.string);
  
  vec_deinit(&self->constants);
  free(self);
}

int bytecode_add_constant_generic(struct bytecode* self, struct constant constant) {
  if (self->constants.length > VM_LIMIT_MAX_CONSTANT)
    return -ENOSPC;
  
  if (vec_push(&self->constants, constant) < 0)
    return -ENOMEM;
  return self->constants.length - 1;
}

int bytecode_add_constant_int(struct bytecode* self, vm_int integer) {
  return bytecode_add_constant_generic(self, (struct constant) {
    .type = BYTECODE_CONSTANT_INTEGER,
    .data.integer = integer
  });
}

int bytecode_add_constant_number(struct bytecode* self, vm_number number) {
  return bytecode_add_constant_generic(self, (struct constant) {
    .type = BYTECODE_CONSTANT_NUMBER,
    .data.integer = number
  });
}

int bytecode_add_constant_string(struct bytecode* self, const char* str) {
  char* cloned = strdup(str);
  if (!cloned)
    return -ENOMEM;
  
  return bytecode_add_constant_generic(self, (struct constant) {
    .type = BYTECODE_CONSTANT_STRING,
    .data.string = cloned
  });
}

int bytecode_get_constant(struct bytecode* self, struct vm* vm, struct value* result, uint32_t index) {
  if (index >= self->constants.length)
    return -EINVAL;

  struct constant* constant = &self->constants.data[index];
  switch (constant->type) {
    case BYTECODE_CONSTANT_INTEGER:
      *result = value_new_int(vm, constant->data.integer);
      break;
    case BYTECODE_CONSTANT_NUMBER:
      *result = value_new_number(vm, constant->data.number);
      break;
    default:
      abort();
  }

  return 0;
}


