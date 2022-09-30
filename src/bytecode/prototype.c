#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "opcodes.h"
#include "prototype.h"
#include "constants.h"
#include "vm_types.h"

struct prototype* prototype_new() {
  struct prototype* self = malloc(sizeof(*self));
  if (!self)
    return NULL;

  self->owner = NULL;
  self->code = NULL;
  self->codeLen = 0;
  self->preDecoded = NULL;
  return self;
}

int prototype_set_code(struct prototype* self, size_t codeLen, vm_instruction* code) {
  if (codeLen >= FLUFFYVM_MAX_CODE)
    return -E2BIG;

  vm_instruction* copied = calloc(codeLen, sizeof(*copied));
  struct instruction* preDecoded = calloc(codeLen, sizeof(*preDecoded));
  if (!copied || !preDecoded)
    return -ENOMEM;
  memcpy(copied, code, sizeof(*copied) * codeLen);
  
  // Pre-decode
  for (vm_instruction_pointer i = 0; i < codeLen; i++)
    if (opcode_decode_instruction(&preDecoded[i], code[i]) < 0)
      goto decode_error;

  // Free old code
  free(self->code);
  free(self->preDecoded);

  self->codeLen = codeLen;
  self->code = copied;
  self->preDecoded = preDecoded;
  return 0;

  decode_error:
  free(copied);
  free(preDecoded);
  return -EINVAL;
}

void prototype_free(struct prototype* self) {
  if (!self)
    return;

  free(self->code);
  free(self);
}

