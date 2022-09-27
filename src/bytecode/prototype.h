#ifndef header_1662272978_47ee9894_beca_4e37_be7d_a1a8c4d5a3ac_prototype_h
#define header_1662272978_47ee9894_beca_4e37_be7d_a1a8c4d5a3ac_prototype_h

#include <stddef.h>

#include "vm_types.h"

struct bytecode;
struct instruction;

struct prototype {
  struct bytecode* owner;

  size_t codeLen;
  vm_instruction* code;  
  struct instruction* preDecoded;
};

struct prototype* prototype_new();

/* On error the prototype remain untouched
 * Errors:
 * -EINVAL: Verification error
 * -ENOMEM: Not enough memory 
 * -E2BIG : Code size too big
 */
int prototype_set_code(struct prototype* self, size_t codeLen, vm_instruction* code);

void prototype_free(struct prototype* self);

#endif

