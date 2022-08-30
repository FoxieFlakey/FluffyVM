#ifndef header_1661670445_7b4872a6_ef25_4bcb_958c_2d4776c953f8_ucontext_h
#define header_1661670445_7b4872a6_ef25_4bcb_958c_2d4776c953f8_ucontext_h

// POSIX thread implementation for
// fibers

#include <pthread.h>

#include "fiber.h"

struct fiber_ucontext {
  struct fiber* owner;

  void (*entryPoint)(struct fiber*);
  volatile enum fiber_command command;

  ucontext_t resumeContext;
  ucontext_t suspendContext;
  
  bool resumeContextInited;
};

struct fiber_ucontext* fiber_impl_ucontext_new(void (*entryPoint)(struct fiber*), struct fiber* owner, struct fiber_ucontext** alsoSaveTo);

void fiber_impl_ucontext_send_command_and_wait(struct fiber_ucontext* self, enum fiber_command command, void** result);
void fiber_impl_ucontext_send_command_completion(struct fiber_ucontext* self, void* result);
enum fiber_command fiber_impl_ucontext_wait_command(struct fiber_ucontext* self);

void fiber_impl_ucontext_free(struct fiber_ucontext* self);

#endif

