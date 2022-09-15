#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "bytecode.h"
#include "prototype.h"
#include "constants.h"
#include "value.h"
#include "vm_types.h"

struct bytecode* bytecode_new() {
  struct bytecode* self = malloc(sizeof(*self));
  if (!self)
    return NULL;

  self->constants = NULL;
  self->constantsLen = 0;
  self->prototypes = NULL;
  self->prototypesLen = 0;
  return self;
}

int bytecode_set_constants(struct bytecode* self, size_t constantsLen, struct constant* constants) {
  if (constantsLen >= FLUFFYVM_MAX_CONSTANTS)
    return -EINVAL;

  struct constant* copied = calloc(constantsLen, sizeof(*copied));
  if (!copied)
    return -ENOMEM;
  memcpy(copied, constants, sizeof(*copied) * constantsLen);

  if (self->constants)
    free(self->constants);
  
  self->constantsLen = constantsLen;
  self->constants = copied;
  return 0;
}

int bytecode_set_prototypes(struct bytecode* self, size_t prototypesLen, struct prototype** prototypes) {
  if (prototypesLen >= FLUFFYVM_MAX_PROTOTYPES)
    return -EINVAL;

  struct prototype** copied = calloc(prototypesLen, sizeof(void*));
  if (!copied)
    return -ENOMEM;
  memcpy(copied, prototypes, sizeof(void*) * prototypesLen);
  
  for (int i = 0; i < prototypesLen; i++)
    copied[i]->owner = self;

  if (self->prototypes) {
    for (int i = 0; i < self->prototypesLen; i++)
      prototype_free(self->prototypes[i]);
    free(self->prototypes);
  }

  self->prototypesLen = prototypesLen;
  self->prototypes = copied;
  return 0;
}

void bytecode_free(struct bytecode* self) {
  if (!self)
    return;

  if (self->prototypes)
    for (int i = 0; i < self->prototypesLen; i++)
      prototype_free(self->prototypes[i]);
  free(self->prototypes);
  free(self->constants);
  free(self);
}

int bytecode_get_constant(struct bytecode* self, struct vm* vm, struct value* result, uint32_t index) {
  if (index > self->constantsLen)
    return -EINVAL;

  struct constant* constant = &self->constants[index];
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




