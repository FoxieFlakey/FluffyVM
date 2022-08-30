#include <Block.h>
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <signal.h>
#include <threads.h>

#include "config.h"

#include "fiber.h"

#include "fiber_impl/ucontext.h"
#include "fiber_impl/posix_thread.h"

#if IS_ENABLED(CONFIG_USE_SETCONTEXT)
# define sendCommandAndWait(self, cmd, result) \
   fiber_impl_ucontext_send_command_and_wait(self->impl.ucontext, cmd, result)
# define waitCommand(self) \
   fiber_impl_ucontext_wait_command(self->impl.ucontext)
# define sendCommandCompletion(self, result) \
   fiber_impl_ucontext_send_command_completion(self->impl.ucontext, result)
#else
# define sendCommandAndWait(self, cmd, result) \
   fiber_impl_posix_send_command_and_wait(self->impl.posix, cmd, result)
# define waitCommand(self) \
   fiber_impl_posix_wait_command(self->impl.posix)
# define sendCommandCompletion(self, result) \
   fiber_impl_posix_send_command_completion(self->impl.posix, result)
#endif


static thread_local struct fiber* currentFiber = NULL;
struct fiber* fiber_get_current() {
  return currentFiber;
}

static void entryPoint(struct fiber* self) {
  struct fiber* parentFiber = currentFiber;
  currentFiber = self;
  
  self->task();
  Block_release(self->task);
  self->task = NULL;

  atomic_store(&self->state, FIBER_DEAD);
  currentFiber = parentFiber;
}

struct fiber* fiber_new(void (^task)()) {
  struct fiber* self = malloc(sizeof(struct fiber));
  if (!self)
    return NULL;

  atomic_init(&self->state, FIBER_SUSPENDED);
  self->task = task;
  self->impl.posix = NULL;
  self->hasResumedForFirst = false;

  if (IS_ENABLED(CONFIG_USE_SETCONTEXT)) {
    self->impl.ucontext = fiber_impl_ucontext_new(entryPoint, self, &self->impl.ucontext);
    if (!self->impl.ucontext)
      goto failure;
  } else {
    self->impl.posix = fiber_impl_posix_posix_new(entryPoint, self, &self->impl.posix);
    if (!self->impl.posix)
      goto failure;
  }

  return self;

  failure:
  fiber_free(self);
  return NULL;
}

void fiber_free(struct fiber* self) {
  if (!self)
    return;

  if (IS_ENABLED(CONFIG_USE_SETCONTEXT))
    fiber_impl_ucontext_free(self->impl.ucontext);
  else 
    fiber_impl_posix_free(self->impl.posix);
 
  if (self->task)
    Block_release(self->task);
  free(self);
}

int fiber_resume(struct fiber* self, enum fiber_state* prevState) {
  enum fiber_state expect = FIBER_SUSPENDED;
  bool exchangeResult = atomic_compare_exchange_strong(&self->state, (int*) &expect, FIBER_RUNNING);
  
  if (prevState)
    *prevState = expect;

  if (!exchangeResult) {
    switch (expect) {
      case FIBER_RUNNING:
        return -EBUSY;
      case FIBER_DEAD:
        return -EINVAL;
      case FIBER_SUSPENDED:
        break;
    }
  }

  sendCommandAndWait(self, FIBER_COMMAND_RESUME, NULL);
  return 0;
}

int fiber_yield() {
  struct fiber* self = fiber_get_current();
  if (!self)
    return -EINVAL;

  atomic_store(&self->state, FIBER_SUSPENDED);

  sendCommandCompletion(self, NULL);
  waitCommand(self);
  return 0;
}

