#include <FluffyGC/v1.h>
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
  for (int i = 0; i < VM_MAX_REGISTERS; i++) {
    self->registers[i] = (struct value) {};
    self->localRefs[i] = NULL;
  }

  return self;
}

void call_state_free(struct call_state* self) {
  for (int i = 0; i < VM_MAX_REGISTERS; i++) 
    fluffygc_v1_delete_local_ref(self->owner->heap, self->localRefs[i]);
  
  free(self);
}

int call_state_set_register(struct call_state* self, int dest, struct value val, bool isFromVM) {
  if (dest < 0 || dest >= VM_MAX_REGISTERS)
    return -EINVAL;
  
  //printf("R(%d) <- %s\n", dest, isFromVM ? "VM" : "Host");
  fluffygc_v1_delete_local_ref(self->owner->heap, self->localRefs[dest]);
  fluffygc_object* ref = value_get_gcobject(self->owner, val);
  if (ref) {
    self->localRefs[dest] = fluffygc_v1_new_local_ref(self->owner->heap, ref);
    if (!self->localRefs[dest])
      return -ENOMEM;
  }
  
  self->registers[dest] = val;
  return 0;
}


