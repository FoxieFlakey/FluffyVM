#ifndef header_1660463374_e6fb7dea_cf03_44ed_bd36_7e8c738a04b1_coroutine_h
#define header_1660463374_e6fb7dea_cf03_44ed_bd36_7e8c738a04b1_coroutine_h

#include <stdbool.h>

struct coroutine;
typedef void (^coroutine_exec_block)(struct coroutine* co, void* arg);

struct coroutine_exec_state {
  bool canRelease;
  coroutine_exec_block block;
};

enum coroutine_state {
  COROUTINE_PAUSED,
  COROUTINE_RUNNING,
  COROUTINE_DEAD
};

// What it is executing
// NATIVE   = A native function executing
// LUA      = A lua function executing
enum coroutine_exec_type {
  COROUTINE_NATIVE,
  COROUTINE_LUA
};

struct coroutine {
  struct vm* F;
  struct coroutine_exec_state exec;

  enum coroutine_state state;
  enum coroutine_exec_type execType;
};

struct coroutine* coroutine_new(struct vm* F, struct coroutine_exec_state exec);
enum coroutine_state coroutine_resume(struct coroutine* self, void* arg);
void coroutine_free(struct coroutine* self);

void coroutine_yield(struct coroutine* self);

#endif

