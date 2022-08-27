#ifndef header_1661430241_75b607df_9e99_4292_87d1_0c886532a33c_fiber_h
#define header_1661430241_75b607df_9e99_4292_87d1_0c886532a33c_fiber_h

#include <stdbool.h>
#include <stdatomic.h>
#include <ucontext.h>

typedef enum {
  FIBER_RUNNING,
  FIBER_SUSPENDED,
  FIBER_DEAD
} fiber_state_t;

struct fiber {
  volatile atomic_int state;
  void (^task)();

  ucontext_t resumeContext;
  ucontext_t suspendContext;
};

bool fiber_yield(struct fiber* self);
bool fiber_resume(struct fiber* self, fiber_state_t* prevState);

struct fiber* fiber_new(void (^task)());
void fiber_free(struct fiber* self);

#endif

