#ifndef header_1650890657_bytecode_h
#define header_1650890657_bytecode_h

#include <stdint.h>

#include "foxgc.h"
#include "value.h"

struct fluffyvm_prototype {
  struct {
    size_t len;
    uint32_t* data;
  } instructions;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_instructions;
};

struct fluffyvm_bytecode {
  struct {
    size_t len;
    struct fluffyvm_prototype** data;
  } prototypes;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_prototypes;
} fluffyvm_bytecode_t;

#endif

