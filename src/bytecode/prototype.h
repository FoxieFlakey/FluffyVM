#ifndef header_1662272978_47ee9894_beca_4e37_be7d_a1a8c4d5a3ac_prototype_h
#define header_1662272978_47ee9894_beca_4e37_be7d_a1a8c4d5a3ac_prototype_h

#include <stddef.h>

#include "vm_types.h"
#include "vec.h"

struct bytecode;
struct instruction;

struct prototype {
  const char* sourceFile;
  const char* prototypeName;
  int definedAtLine;
  int definedAtColumn;
  
  struct bytecode* owner;

  vec_t(vm_instruction) code;  
  vec_t(struct instruction) preDecoded;
};

struct prototype* prototype_new();
void prototype_free(struct prototype* self);

#endif

