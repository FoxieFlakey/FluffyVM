#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bytecode.h"
#include "fluffyvm.h"
#include "fluffyvm_types.h"
#include "value.h"
#include "format/bytecode.pb-c.h"

#define create_descriptor(name, structure, ...) do { \
  size_t offsets[] = __VA_ARGS__; \
  vm->bytecodeStaticData->name = foxgc_api_descriptor_new(vm->heap, sizeof(offsets) / sizeof(offsets[0]), offsets, sizeof(structure)); \
  if (vm->bytecodeStaticData->name == NULL) \
    return false; \
} while (0)

#define free_descriptor(name) do { \
  if (vm->bytecodeStaticData->name) \
    foxgc_api_descriptor_remove(vm->bytecodeStaticData->name); \
} while(0)

bool bytecode_init(struct fluffyvm* vm) {
  vm->bytecodeStaticData = malloc(sizeof(*vm->bytecodeStaticData));
  create_descriptor(desc_bytecode, struct fluffyvm_bytecode, {
    offsetof(struct fluffyvm_bytecode, gc_this),
    offsetof(struct fluffyvm_bytecode, gc_prototypes),
  });
  
  create_descriptor(desc_prototype, struct fluffyvm_bytecode, {
    offsetof(struct fluffyvm_prototype, gc_this),
    offsetof(struct fluffyvm_prototype, gc_bytecode),
    offsetof(struct fluffyvm_prototype, gc_instructions),
  });

  return true;
}

void bytecode_cleanup(struct fluffyvm* vm) {
  free_descriptor(desc_bytecode);
  free_descriptor(desc_prototype);
  free(vm->bytecodeStaticData);
}

struct bytecode* bytecode_load(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, void* data, size_t len) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->bytecodeStaticData->desc_bytecode, NULL);
  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->valueStaticData->outOfMemoryString);
    return NULL;
  }
  struct bytecode* this = foxgc_api_object_get_data(obj);
  
  // Decode
  FluffyVmFormat__Bytecode__Bytecode* bytecode = fluffy_vm_format__bytecode__bytecode__unpack(NULL, len, data);
  
  fluffy_vm_format__bytecode__bytecode__free_unpacked(bytecode, NULL);
  return this;
}







