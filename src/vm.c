#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "vm.h"
#include "config.h"

struct vm* vm_new(fluffygc_state* heap) {
  struct vm* self = malloc(sizeof(*self));
  if (!self)
    goto failure;

  self->heap = heap;
  self->osThreadLocalDataKeyInited = false;

  atomic_init(&self->threadAttachedCount, 0);
  if (pthread_key_create(&self->osThreadLocalDataKey, NULL) != 0)
    goto failure;
  self->osThreadLocalDataKeyInited = true;

  return self;

  failure:
  vm_free(self);
  return NULL;
}

void vm_free(struct vm* self) {
  if (!self)
    return;

  if (atomic_load(&self->threadAttachedCount) > 0)
    abort();

  if (self->osThreadLocalDataKeyInited)
    pthread_key_delete(self->osThreadLocalDataKey);
  free(self);
}

