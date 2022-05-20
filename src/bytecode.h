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
  
  // Debug info
  size_t lineInfo_len;
  int* lineInfo;

  // Guarantee to be null terminated string
  // (if no source file name provided in 
  // debug info its 0 byte length)
  struct value sourceFile;
  foxgc_object_t* sourceFileObject;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_instructions;
  foxgc_object_t* gc_bytecode;
  foxgc_object_t* gc_prototypes;
  foxgc_object_t* gc_lineInfo;
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




