#ifndef header_1662074719_23d2cbd0_bdf7_4140_a0aa_16526720c194_call_state_h
#define header_1662074719_23d2cbd0_bdf7_4140_a0aa_16526720c194_call_state_h

#include <errno.h>
#include <stdbool.h>

#include "performance.h"
#include "value.h"
#include "vm_limits.h"
#include "attributes.h"
#include "constants.h"

struct vm;
struct prototype;

struct call_state {
  struct vm* owner;

  struct prototype* proto;
  struct value registers[VM_MAX_REGISTERS];
};

struct call_state* call_state_new(struct vm* owner, struct prototype* proto);
void call_state_free(struct call_state* self);

/* Error:
 * -EINVAL: Invalid argument
 */
static int call_state_move_register(struct call_state* self, int dest, int src);

/* Error:
 * -EINVAL: Invalid argument
 * -ENOMEM: Not enough memory
 */
static inline int call_state_set_register(struct call_state* self, int dest, struct value val, bool isFromVM);

/* Error:
 * -EINVAL: Invalid argument
 * -ENOMEM: Not enough memory
 */
static inline int call_state_get_register(struct call_state* self, struct value* dest, int src, bool isFromVM);

////////////////////////////////
/// Inlined stuff below here ///
////////////////////////////////

static inline int call_state_set_register(struct call_state* self, int dest, struct value val, bool isFromVM) {
  if (dest < 0 || dest >= VM_MAX_REGISTERS)
    return -EINVAL;
  
  //printf("R(%d) <- %s\n", dest, isFromVM ? "VM" : "Host");
  self->registers[dest] = val;
  return 0;
}

static inline int call_state_get_register(struct call_state* self, struct value* dest, int src, bool isFromVM) {
  if (src < 0 || src >= VM_MAX_REGISTERS)
    return -EINVAL;
  
  //printf("%s <- R(%d)\n", isFromVM ? "VM" : "Host", src);
  *dest = self->registers[src];
  return 0;
}

static inline int call_state_move_register(struct call_state* self, int dest, int src) {
  if (src < 0 || dest < 0 ||
      src >= VM_MAX_REGISTERS ||
      dest >= VM_MAX_REGISTERS) {
    return -EINVAL;
  }

  //printf("R(%d) <- R(%d)\n", dest, src);
  self->registers[dest] = self->registers[src];
  return 0;
}

#endif

