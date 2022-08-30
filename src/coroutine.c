#include <stdlib.h>
#include <Block.h>

#include "coroutine.h"
#include "fiber.h"

struct coroutine* coroutine_new(struct vm* F, struct coroutine_exec_state exec) {
  struct coroutine* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  self->F = F;
  self->exec = exec;
  self->fiber = fiber_new(Block_copy(^void () {
    self->exec.block();
    if (self->exec.canRelease)
      Block_release(self->exec.block);
  }));
  
  if (!self->fiber)
    goto failure;

  return self;

  failure:
  coroutine_free(self);
  return NULL;
}

void coroutine_free(struct coroutine* self) {
  if (!self)
    return;
  fiber_free(self->fiber);
  if (self->exec.block)
    Block_release(self->exec.block);
  free(self);
}

