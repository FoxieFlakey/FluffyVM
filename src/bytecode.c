#include <pthread.h>
#include <stdlib.h>

#include "bytecode.h"
#include "fluffyvm.h"
#include "fluffyvm_types.h"
#include "parser/cjson.h"

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

// cJSON not fully thread safe and
// i dont want read list of the part
// that not thread safe
static pthread_mutex_t cjson_lock = PTHREAD_MUTEX_INITIALIZER;

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










