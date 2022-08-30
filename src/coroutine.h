#ifndef header_1660463374_e6fb7dea_cf03_44ed_bd36_7e8c738a04b1_coroutine_h
#define header_1660463374_e6fb7dea_cf03_44ed_bd36_7e8c738a04b1_coroutine_h

#include <stdbool.h>
#include <errno.h>

struct fiber;
struct coroutine;

typedef void (^coroutine_exec_block)();

struct coroutine_exec_state {
  bool canRelease;
  coroutine_exec_block block;
};

struct coroutine {
  struct vm* F;
  struct coroutine_exec_state exec;
  struct fiber* fiber;
};

struct coroutine* coroutine_new(struct vm* F, struct coroutine_exec_state exec);
void coroutine_free(struct coroutine* self);

#endif

