#ifndef header_1650890657_bytecode_h
#define header_1650890657_bytecode_h

#include <stdint.h>
#include <stdbool.h>

#include "fluffyvm.h"
#include "foxgc.h"

struct fluffyvm_prototype {
  struct fluffyvm_bytecode* bytecode;
  struct {
    size_t len;
    uint32_t* data;
  } instructions; 

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_instructions;
  foxgc_object_t* gc_bytecode;
};

struct fluffyvm_bytecode {
  size_t prototypes_len;
  foxgc_object_t** prototypes;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_prototypes;
};

bool bytecode_init(struct fluffyvm* vm); 
void bytecode_cleanup(struct fluffyvm* vm); 

// Load bytecode from JSON
struct fluffyvm_bytecode* bytecode_from_json(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, const char* string, size_t len);

#endif

