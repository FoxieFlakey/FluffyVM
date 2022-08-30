#ifndef header_1661667225_d81b69ae_010e_438f_926b_f0e7b7485e23_posix_thread_h
#define header_1661667225_d81b69ae_010e_438f_926b_f0e7b7485e23_posix_thread_h

// POSIX thread implementation for
// fibers

#include <pthread.h>

#include "fiber.h"

struct fiber_posix {
  struct fiber* owner;
  void (*entryPoint)(struct fiber*);

  pthread_t thread;
  pthread_barrier_t barrier;

  bool firstCommand;

  enum fiber_command command;
  bool commandReceived;
  pthread_cond_t commandReceivedCond;
  pthread_mutex_t commandReceivedLock;
  
  bool commandCompleted;
  pthread_cond_t commandCompletedCond;
  pthread_mutex_t commandCompletedLock;
  
  bool threadInited;
  bool barrierInited;
  bool commandReceivedLockInited;
  bool commandReceivedCondInited;
  bool commandCompletedLockInited;
  bool commandCompletedCondInited;
};

struct fiber_posix* fiber_impl_posix_posix_new(void (*entryPoint)(struct fiber*), struct fiber* owner, struct fiber_posix** alsoSaveTo);

void fiber_impl_posix_send_command_and_wait(struct fiber_posix* self, enum fiber_command command, void** result);
void fiber_impl_posix_send_command_completion(struct fiber_posix* self, void* result);
enum fiber_command fiber_impl_posix_wait_command(struct fiber_posix* self);

void fiber_impl_posix_free(struct fiber_posix* self);

#endif

