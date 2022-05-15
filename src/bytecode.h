#ifndef header_1650890657_bytecode_h
#define header_1650890657_bytecode_h

#include <stdint.h>
#include <stdbool.h>

#include "fluffyvm.h"
#include "foxgc.h"

// Each instruction is 64-bit
typedef uint64_t fluffyvm_instruction_t;

struct fluffyvm_prototype {
  struct fluffyvm_bytecode* bytecode;
  
  size_t instructions_len;
  fluffyvm_instruction_t* instructions;
  
  size_t prototypes_len;
  // struct fluffyvm_prototype*
  foxgc_object_t** prototypes;
  
  foxgc_object_t* gc_this;
  foxgc_object_t* gc_instructions;
  foxgc_object_t* gc_bytecode;
  foxgc_object_t* gc_prototypes;
};

struct fluffyvm_bytecode {
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

// Getters
struct value bytecode_get_constant(struct fluffyvm* vm, struct fluffyvm_bytecode* bytecode, foxgc_root_reference_t** rootRef, int index);
struct fluffyvm_prototype* bytecode_prototype_get_prototype(struct fluffyvm* vm, struct fluffyvm_prototype* prototype, foxgc_root_reference_t** rootRef, int index);

#endif




