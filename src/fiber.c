#include <Block.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>

#include "config.h"

#ifdef FLUFFYVM_ASAN_ENABLED
# include <sanitizer/asan_interface.h>
#endif

#include "fiber.h"

// VERY CRONGED
// SIGSTKSZ IS UNDEFINED
#define SIGSTKSZ (1024 * 104 * 8)

static inline void sanitizer_start_switch_fiber(void* bottom, size_t size) {
# ifdef FLUFFYVM_ASAN_ENABLED
  __sanitizer_start_switch_fiber(NULL, bottom, size);
# endif
}

static inline void sanitizer_finish_switch_fiber() {
# ifdef FLUFFYVM_ASAN_ENABLED
   __sanitizer_finish_switch_fiber(NULL, NULL, NULL);
# endif
}

static void entryPoint(struct fiber* fiber, runnable_t task) {
  sanitizer_finish_switch_fiber();
  
  task();
  Block_release(task);
  fiber->task = NULL;

  pthread_mutex_lock(&fiber->lock);
  fiber->state = FIBER_DEAD;
  pthread_mutex_unlock(&fiber->lock);
  
  sanitizer_start_switch_fiber(fiber->suspendContext.uc_stack.ss_sp, 
                               fiber->suspendContext.uc_stack.ss_size);
  swapcontext(&fiber->resumeContext, &fiber->suspendContext);
  
  // Its illegal to reach here
  abort();
}

struct fiber* fiber_new(runnable_t task) {
  struct fiber* fiber = malloc(sizeof(struct fiber));
  
  fiber->state = FIBER_SUSPENDED;
  
  fiber->resumeContext.uc_stack.ss_sp = malloc(SIGSTKSZ);
  fiber->resumeContext.uc_stack.ss_flags = 0;
  fiber->resumeContext.uc_stack.ss_size = SIGSTKSZ;
  fiber->resumeContext.uc_link = NULL;
  fiber->task = task;

  getcontext(&fiber->resumeContext);
  makecontext(&fiber->resumeContext, (void(*)()) entryPoint, 2, fiber, task);
  pthread_mutex_init(&fiber->lock, NULL);

  return fiber;
}

void fiber_free(struct fiber* fiber) {
  pthread_mutex_destroy(&fiber->lock);
  free(fiber->resumeContext.uc_stack.ss_sp);
  if (fiber->task)
    Block_release(fiber->task);
  free(fiber);
}

bool fiber_resume(struct fiber* fiber, fiber_state_t* prevState) {
  pthread_mutex_lock(&fiber->lock);
  if (prevState)
    *prevState = fiber->state;
  
  if (fiber->state != FIBER_SUSPENDED) {
    pthread_mutex_unlock(&fiber->lock);
    return false;
  }
  
  fiber->state = FIBER_RUNNING;
  pthread_mutex_unlock(&fiber->lock);
 
  sanitizer_start_switch_fiber(&fiber->resumeContext.uc_stack.ss_sp, 
                                fiber->resumeContext.uc_stack.ss_size);
  swapcontext(&fiber->suspendContext, &fiber->resumeContext);
  sanitizer_finish_switch_fiber();

  pthread_mutex_lock(&fiber->lock);
  if (fiber->state == FIBER_DEAD) {
    // At here we are sure that stack wont be 
    // used anymore because the fiber is done
    // executing the code
    free(fiber->resumeContext.uc_stack.ss_sp);
    fiber->resumeContext.uc_stack.ss_sp = NULL;
  }

  pthread_mutex_unlock(&fiber->lock);
  return true;
}

bool fiber_yield(struct fiber* fiber) {
  pthread_mutex_lock(&fiber->lock);
  
  // This is treated as error in caller code
  // Its illegal to yield dead fiber
  assert(fiber->state != FIBER_DEAD);

  if (fiber->state != FIBER_RUNNING) {
    pthread_mutex_unlock(&fiber->lock);
    return false;
  }
  
  fiber->state = FIBER_SUSPENDED;
  pthread_mutex_unlock(&fiber->lock);

  sanitizer_start_switch_fiber(fiber->suspendContext.uc_stack.ss_sp, 
                               fiber->suspendContext.uc_stack.ss_size);
  swapcontext(&fiber->resumeContext, &fiber->suspendContext);
  sanitizer_finish_switch_fiber();
  
  return true;
}

