#include <stdlib.h>
#include <errno.h>

#include "call_state.h"
#include "vm_limits.h"
#include "vm.h"
#include "constants.h"

struct call_state* call_state_new(struct vm* owner, struct prototype* proto) {
  struct call_state* self = malloc(sizeof(*self));
  if (!self)
    return NULL;

  self->owner = owner;
  self->proto = proto;
  for (int i = 0; i < VM_MAX_REGISTERS; i++)
    self->registers[i] = (struct value) {};

  return self;
}

void call_state_free(struct call_state* self) {
  free(self);
}




