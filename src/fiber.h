#ifndef header_1652612886_coroutine_h
#define header_1652612886_coroutine_h

#include <stdbool.h>
#include <ucontext.h>
#include <pthread.h>

#include "util/functional/functional.h"

typedef enum {
  FIBER_RUNNING,
  FIBER_SUSPENDED,
  FIBER_DEAD
} fiber_state_t;

struct fiber {
  // To guard the state variable
  pthread_mutex_t lock;

  // Compiler may optimize this away
  volatile fiber_state_t state;
  runnable_t task;

  ucontext_t resumeContext;
  ucontext_t suspendContext;
};

bool fiber_yield(struct fiber* fiber);
bool fiber_resume(struct fiber* fiber, fiber_state_t* prevState);

struct fiber* fiber_new(runnable_t task);
void fiber_free(struct fiber* fiber);

#endif

