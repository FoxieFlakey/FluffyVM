#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "opcodes.h"
#include "prototype.h"
#include "vm_limits.h"
#include "vec.h"
#include "vm_types.h"
#include "bug.h"

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

  if (vec_reserve(&self->code, codeLen) < 0 || vec_reserve(&self->preDecoded, codeLen) < 0)
    return -ENOMEM;
  for (int i = 0; i < codeLen; i++)
    if (vec_push(&self->code, code[i]) < 0)
      BUG(); // Already reserved memory this cant fail
  
  int res = 0;
  
  // Pre-decode
  // TODO: Somehow make this code revert overwritten data to original on failure 
  for (vm_instruction_pointer i = 0; i < codeLen; i++) {
    struct instruction tmp;
    if (opcode_decode_instruction(&tmp, code[i]) < 0) {
      res = -EINVAL;
      goto decode_error;
    }
    
    if (vec_push(&self->preDecoded, tmp) < 0)
      BUG(); // Already reserved memory this cant fail
  }

decode_error:
  return res; 
}

void prototype_free(struct prototype* self) {
  if (!self)
    return;

  vec_deinit(&self->code);
  vec_deinit(&self->preDecoded);
  free(self);
}

