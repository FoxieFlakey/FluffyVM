#include "fluffyvm_types.h"
#include "closure.h"
#include <stddef.h>

#define create_descriptor(name, structure, ...) do { \
  size_t offsets[] = __VA_ARGS__; \
  vm->closureStaticData->name = foxgc_api_descriptor_new(vm->heap, sizeof(offsets) / sizeof(offsets[0]), offsets, sizeof(structure)); \
  if (vm->closureStaticData->name == NULL) \
    return false; \
} while (0)

#define free_descriptor(name) do { \
  if (vm->closureStaticData->name) \
    foxgc_api_descriptor_remove(vm->closureStaticData->name); \
} while(0)

bool closure_init(struct fluffyvm* vm) {
  vm->closureStaticData = malloc(sizeof(*vm->closureStaticData));
  if (!vm->closureStaticData)
    return false;

  create_descriptor(desc_closure, struct fluffyvm_closure, {
    offsetof(struct fluffyvm_closure, gc_this), 
    offsetof(struct fluffyvm_closure, gc_prototype) 
  });

  return true;
}

#define CLOSURE_OFFSET_THIS (0)
#define CLOSURE_OFFSET_PROTOTYPE (1)

void closure_cleanup(struct fluffyvm* vm) {
  if (vm->closureStaticData) {
    free_descriptor(desc_closure);
    free(vm->closureStaticData);
  }
}

struct fluffyvm_closure* closure_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, struct fluffyvm_prototype* prototype) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->closureStaticData->desc_closure, NULL);
  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }
  struct fluffyvm_closure* this = foxgc_api_object_get_data(obj);
  // Write to this->gc_this
  foxgc_api_write_field(obj, CLOSURE_OFFSET_THIS, obj);

  this->prototype = prototype;
  foxgc_api_write_field(obj, CLOSURE_OFFSET_PROTOTYPE, prototype->gc_this);

  return this;
  
  /* error:
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), *rootRef);
  *rootRef = NULL;
  return NULL; */
}








