#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "opcodes.h"
#include "prototype.h"
#include "vm_limits.h"
#include "vec.h"
#include "vm_types.h"

struct prototype* prototype_new() {
  struct prototype* self = malloc(sizeof(*self));
  if (!self)
    return NULL;

  self->owner = NULL;
  vec_init(&self->code);
  vec_init(&self->preDecoded);
  return self;
}

int prototype_set_code(struct prototype* self, size_t codeLen, vm_instruction* code) {
  if (codeLen >= VM_LIMIT_MAX_CODE)
    return -E2BIG;

  if (!vec_reserve(&self->code, codeLen) || !vec_reserve(&self->preDecoded, codeLen))
    return -ENOMEM;
  memcpy(self->code.data, code, sizeof(*self->code.data) * codeLen);
  
  // Pre-decode
  for (vm_instruction_pointer i = 0; i < codeLen; i++)
    if (opcode_decode_instruction(&self->preDecoded.data[i], code[i]) < 0)
      goto decode_error;

  // Free old code
  vec_deinit(&self->code);
  vec_deinit(&self->preDecoded);
  return 0;

decode_error:
  return -EINVAL;
}

void prototype_free(struct prototype* self) {
  if (!self)
    return;

  vec_deinit(&self->code);
  vec_deinit(&self->preDecoded);
  free(self);
}

