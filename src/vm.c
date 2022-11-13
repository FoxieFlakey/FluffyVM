#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "bug.h"
#include "vm.h"
#include "config.h"
#include "vm_string.h"

struct vm* vm_new(fluffygc_state* heap) {
  struct vm* self = malloc(sizeof(*self));
  if (!self)
    goto failure;

  self->heap = heap;
  self->osThreadLocalDataKeyInited = false;
# define X(name, ...) self->name ## Inited = false;
  VM_SUBSYSTEMS
# undef X

  atomic_init(&self->threadAttachedCount, 0);
  if (pthread_key_create(&self->osThreadLocalDataKey, NULL) != 0)
    goto failure;
  self->osThreadLocalDataKeyInited = true;

# define X(name, ...) if (name ## _init(self) < 0) \
    goto failure; \
  self->name ## Inited = true;
  VM_SUBSYSTEMS
# undef X

  return self;

failure:
  vm_free(self);
  return NULL;
}

void vm_free(struct vm* self) {
  if (!self)
    return;

  if (atomic_load(&self->threadAttachedCount) > 0)
    BUG();

# define X(name, ...) if (self->name ## Inited) \
    name ## _cleanup(self); 
  VM_SUBSYSTEMS
# undef X

  if (self->osThreadLocalDataKeyInited)
    pthread_key_delete(self->osThreadLocalDataKey);
  free(self);
}

