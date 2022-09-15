#ifndef header_1660463417_214133f8_ad42_4a8b_9306_246d4dd87889_value_h
#define header_1660463417_214133f8_ad42_4a8b_9306_246d4dd87889_value_h

#include <stdbool.h>

#include "vm_types.h"

struct vm;
enum value_types {
  VALUE_NONE,
  VALUE_NIL,
  VALUE_INTEGER,
  VALUE_NUMBER
};

struct value {
  enum value_types type;
  union {
    vm_int integer;
    vm_number number;
  } data;
};

static inline struct value value_new_none(struct vm* vm);
static inline struct value value_new_nil(struct vm* vm);
static inline struct value value_new_int(struct vm* vm, vm_int integer);
static inline struct value value_new_number(struct vm* vm, vm_number integer);

typedef int (*math_operation_func)(struct vm*, struct value*, struct value, struct value);

/* Error:
 * -EFAULT: Error thrown
 * -ENOMEM: No memory
 */
int value_add(struct vm* vm, struct value* res, struct value a, struct value b);
int value_sub(struct vm* vm, struct value* res, struct value a, struct value b);
int value_mul(struct vm* vm, struct value* res, struct value a, struct value b);
int value_div(struct vm* vm, struct value* res, struct value a, struct value b);
int value_mod(struct vm* vm, struct value* res, struct value a, struct value b);
int value_pow(struct vm* vm, struct value* res, struct value a, struct value b);

bool value_is_less(struct vm* vm, struct value a, struct value b);
bool value_is_equal(struct vm* vm, struct value a, struct value b);

////////////////////////////////
/// Inlined stuff below here ///
////////////////////////////////

// Inlined functions
static inline struct value value_new_none(struct vm* vm) {
  return (struct value) {
    .type = VALUE_NONE
  };
}

static inline struct value value_new_nil(struct vm* vm) {
  return (struct value) {
    .type = VALUE_NIL
  };
}

static inline struct value value_new_int(struct vm* vm, vm_int integer) {
  return (struct value) {
    .type = VALUE_INTEGER,
    .data.integer = integer
  };
}

static inline struct value value_new_number(struct vm* vm, vm_number number) {
  return (struct value) {
    .type = VALUE_NUMBER,
    .data.number = number
  };
}

#endif

