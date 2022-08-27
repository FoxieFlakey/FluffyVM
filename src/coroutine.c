#include <stdlib.h>
#include <Block.h>
#include "coroutine.h"

struct coroutine* coroutine_new(struct vm* F, struct coroutine_exec_state exec) {
  struct coroutine* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  self->F = F;
  self->exec = exec;
  self->execType = COROUTINE_NATIVE;
  self->state = COROUTINE_PAUSED;

  return self;
}

enum coroutine_state coroutine_resume(struct coroutine* self, void* arg) {
  if (self->state == COROUTINE_DEAD)
    return COROUTINE_DEAD;

  self->state = COROUTINE_RUNNING;

  while (self->exec.block && self->state == COROUTINE_RUNNING) {
    struct coroutine_exec_state exec = self->exec;
    self->exec = (struct coroutine_exec_state) {};
    
    exec.block(self, arg);
    if (exec.canRelease)
      Block_release(exec.block);
  }

  // The thing running in block
  // doesnt pass any continuation
  if (!self->exec.block)
    self->state = COROUTINE_DEAD;

  return self->state;
}

void coroutine_yield(struct coroutine* self) {
  self->state = COROUTINE_PAUSED;
}

void coroutine_free(struct coroutine* self) {
  if (!self)
    return;
  if (self->exec.block)
    Block_release(self->exec.block);

  free(self);
}

