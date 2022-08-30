#ifndef header_1661430241_75b607df_9e99_4292_87d1_0c886532a33c_fiber_h
#define header_1661430241_75b607df_9e99_4292_87d1_0c886532a33c_fiber_h

#include <stdbool.h>
#include <stdatomic.h>
#include <ucontext.h>
#include <errno.h>

struct fiber_posix;
struct fiber_ucontext;

enum fiber_state {
  FIBER_RUNNING,
  FIBER_SUSPENDED,
  FIBER_DEAD
};

enum fiber_command {
  FIBER_COMMAND_RESUME,
  FIBER_COMMAND_TERMINATE
};

struct fiber {
  volatile atomic_int state;
  void (^task)();
  void* udata;
  bool hasResumedForFirst;

  union {
    struct fiber_posix* posix;
    struct fiber_ucontext* ucontext;
  } impl;
};

// Errors:
// -EINVAL: Yielding outside of fiber
int fiber_yield();

// Errors:
// -EINVAL: Resuming dead fiber
// -EBUSY: Resuming already running fiber
int fiber_resume(struct fiber* self, enum fiber_state* prevState);

struct fiber* fiber_new(void (^task)());
void fiber_free(struct fiber* self);

// Get current fiber which current thread
// is in
struct fiber* fiber_get_current();

#endif

