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
  struct fluffyvm_prototype* mainPrototype;
  
  size_t constants_len;
  struct value* constants;
  foxgc_object_t** constantsObject;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_mainPrototype;
  foxgc_object_t* gc_constants;
  foxgc_object_t* gc_constantsObject;
};

bool bytecode_init(struct fluffyvm* vm); 
void bytecode_cleanup(struct fluffyvm* vm); 

// `data` is pointing to buffer with size of `len`
// containing ProtoBuf encoded `message Bytecode`
struct fluffyvm_bytecode* bytecode_load(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, void* data, size_t len);

#endif




