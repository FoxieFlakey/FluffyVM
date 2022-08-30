#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <threads.h>
#include <ucontext.h>

#include "fiber.h"
#include "fiber_impl/ucontext.h"
#include "config.h"

#if IS_ENABLED(CONFIG_ASAN)
# include <sanitizer/asan_interface.h>
#endif

#if IS_ENABLED(CONFIG_USE_SETCONTEXT)
# error setcontext support not ready, yet please use posix threads
#endif

static inline void sanitizer_start_switch_fiber(void* bottom, size_t size) {
# if IS_ENABLED(CONFIG_ASAN)
  __sanitizer_start_switch_fiber(NULL, bottom, size);
# endif
}

static inline void sanitizer_finish_switch_fiber() {
# if IS_ENABLED(CONFIG_ASAN)
   __sanitizer_finish_switch_fiber(NULL, NULL, NULL);
# endif
}

#if !IS_ENABLED(CONFIG_USE_SETCONTEXT)
// Replace the function with one that aborts

static int getcontext_fake(ucontext_t* ctx) {
  abort();
}
static int swapcontext_fake(ucontext_t* restrict oldCtx, const ucontext_t* restrict newCtx) {
  abort();
}
static void makecontext_fake(ucontext_t* ctx, void (*func)(), int argCount, ...) {
  abort();
}

#define getcontext getcontext_fake
#define swapcontext swapcontext_fake
#define makecontext makecontext_fake
#endif

static thread_local struct fiber_ucontext* ctxEntryPoint_context = NULL;
static void ctxEntryPoint() {
  sanitizer_finish_switch_fiber();
  
  struct fiber_ucontext* self = ctxEntryPoint_context;
  ctxEntryPoint_context = NULL;
  
  self->entryPoint(self->owner);

  fiber_impl_ucontext_wait_command(self);
  abort();
}

struct fiber_ucontext* fiber_impl_ucontext_new(void (*entryPoint)(struct fiber*), struct fiber* owner, struct fiber_ucontext** alsoSaveTo) {
  struct fiber_ucontext* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  *alsoSaveTo = self;
  
  self->owner = owner;  
  self->entryPoint = entryPoint;
  self->resumeContext = (ucontext_t) {};
  self->suspendContext = (ucontext_t) {};
  self->resumeContextInited = false;
  
  if (getcontext(&self->resumeContext) != 0)
    goto failure;
  self->resumeContextInited = true;
   
  self->resumeContext.uc_stack.ss_sp = calloc(1, SIGSTKSZ);
  if (self->resumeContext.uc_stack.ss_sp == NULL)
    goto failure;

  self->resumeContext.uc_stack.ss_flags = 0;
  self->resumeContext.uc_stack.ss_size = SIGSTKSZ;
  self->resumeContext.uc_link = NULL;
  
  // Somehow convert ptr to vararg of ints???
  makecontext(&self->resumeContext, ctxEntryPoint, 0);  
 
  ctxEntryPoint_context = self;
  
  //fiber_impl_ucontext_send_command_and_wait(self, FIBER_COMMAND_RESUME, NULL);
  return self;

  failure:
  fiber_impl_ucontext_free(self);
  return NULL;
}

void fiber_impl_ucontext_send_command_and_wait(struct fiber_ucontext* self, enum fiber_command command, void** result) {
  self->command = command;
    
  sanitizer_start_switch_fiber(&self->resumeContext.uc_stack.ss_sp, 
                                self->resumeContext.uc_stack.ss_size);
  swapcontext(&self->suspendContext, &self->resumeContext);
  sanitizer_finish_switch_fiber();
  
  if (atomic_load(&self->owner->state) == FIBER_DEAD) {
    // At here we are sure that stack wont be 
    // used anymore because the fiber is done
    // executing the code
    free(self->resumeContext.uc_stack.ss_sp);
    self->resumeContext.uc_stack.ss_sp = NULL;
  }
}

void fiber_impl_ucontext_send_command_completion(struct fiber_ucontext* self, void* result) {  
}

enum fiber_command fiber_impl_ucontext_wait_command(struct fiber_ucontext* self) {
  sanitizer_start_switch_fiber(self->suspendContext.uc_stack.ss_sp, 
                               self->suspendContext.uc_stack.ss_size);
  swapcontext(&self->resumeContext, &self->suspendContext);
  sanitizer_finish_switch_fiber();

  return self->command;
}

void fiber_impl_ucontext_free(struct fiber_ucontext* self) {
  if (!self)
    return;

  free(self->resumeContext.uc_stack.ss_sp);
  free(self);
} 



