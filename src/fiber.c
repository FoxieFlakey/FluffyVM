/*
Commented because wondering why
makecontext and swapcontext removed
from posix 2008 but getcontext and
setcontext exists

#include <Block.h>
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"

#if IS_ENABLED(CONFIG_ASAN_ENABLED)
# include <sanitizer/asan_interface.h>
#endif

#include "fiber.h"

static inline void sanitizer_start_switch_fiber(void* bottom, size_t size) {
# if IS_ENABLED(CONFIG_ASAN_ENABLED)
  __sanitizer_start_switch_fiber(NULL, bottom, size);
# endif
}

static inline void sanitizer_finish_switch_fiber() {
# if IS_ENABLED(CONFIG_ASAN_ENABLED)
   __sanitizer_finish_switch_fiber(NULL, NULL, NULL);
# endif
}

static void entryPoint(struct fiber* self, void (^task)()) {
  sanitizer_finish_switch_fiber();
  
  task();
  Block_release(task);
  self->task = NULL;

  self->state = FIBER_DEAD;
  sanitizer_start_switch_fiber(self->suspendContext.uc_stack.ss_sp, 
                               self->suspendContext.uc_stack.ss_size);
  swapcontext(&self->resumeContext, &self->suspendContext);
  
  // Its illegal to reach here
  abort();
}

struct fiber* fiber_new(void (^task)()) {
  struct fiber* self = malloc(sizeof(struct fiber));
  if (!self)
    return NULL;

  self->state = FIBER_SUSPENDED;
  
  if (getcontext(&self->resumeContext) < 0)
    goto failure;
  
  self->resumeContext.uc_stack.ss_sp = calloc(1, SIGSTKSZ);
  if (self->resumeContext.uc_stack.ss_sp == NULL)
    goto failure;

  self->resumeContext.uc_stack.ss_flags = 0;
  self->resumeContext.uc_stack.ss_size = SIGSTKSZ;
  self->resumeContext.uc_link = NULL;
  self->task = task;

  makecontext(&self->resumeContext, (void(*)()) entryPoint, 2, self, task);

  return self;

  failure:
  fiber_free(self);
  return NULL;
}

void fiber_free(struct fiber* self) {
  if (!self)
    return;

  if (self->task)
    Block_release(self->task);
  free(self->resumeContext.uc_stack.ss_sp);
  free(self);
}

bool fiber_resume(struct fiber* self, fiber_state_t* prevState) {
  if (prevState)
    *prevState = self->state;
  
  fiber_state_t expect = FIBER_SUSPENDED;
  if (!atomic_compare_exchange_strong(&self->state, (int*) &expect, FIBER_RUNNING))
    return false;
   
  sanitizer_start_switch_fiber(&self->resumeContext.uc_stack.ss_sp, 
                                self->resumeContext.uc_stack.ss_size);
  swapcontext(&self->suspendContext, &self->resumeContext);
  sanitizer_finish_switch_fiber();

  if (self->state == FIBER_DEAD) {
    // At here we are sure that stack wont be 
    // used anymore because the fiber is done
    // executing the code
    free(self->resumeContext.uc_stack.ss_sp);
    self->resumeContext.uc_stack.ss_sp = NULL;
  }

  assert(self->state == FIBER_SUSPENDED);
  return true;
}

bool fiber_yield(struct fiber* self) { 
  fiber_state_t expect = FIBER_RUNNING;
  if (!atomic_compare_exchange_strong(&self->state, (int*) &expect, FIBER_SUSPENDED)) {
    fiber_state_t prev = expect;
    
    if (prev == FIBER_DEAD)
      abort(); // Attempt to suspend dead fiber
               // abort because it doesnt make any
               // sense to yield dead fiber
    return false;
  }

  sanitizer_start_switch_fiber(self->suspendContext.uc_stack.ss_sp, 
                               self->suspendContext.uc_stack.ss_size);
  swapcontext(&self->resumeContext, &self->suspendContext);
  sanitizer_finish_switch_fiber();
  
  return true;
}
*/

