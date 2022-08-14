#include <stdlib.h>
#include "vm.h"

struct fluffyvm* vm_new(fluffygc_state* heap) {
  struct fluffyvm* self = malloc(sizeof(*self));
  if (!self)
    goto failure;

  self->heap = heap;
  return self;

  failure:
  vm_free(self);
  return NULL;
}

void vm_free(struct fluffyvm* vm) {
  if (!vm)
    return;

  free(vm);
}

