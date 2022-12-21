#ifndef header_1660463417_214133f8_ad42_4a8b_9306_246d4dd87889_value_h
#define header_1660463417_214133f8_ad42_4a8b_9306_246d4dd87889_value_h

#include <FluffyGC/v1.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#include "vm_array_primitive.h"
#include "vm_string.h"
#include "vm_types.h"

struct vm;
struct string_gcobject;

enum value_types {
  VALUE_NONE,
  VALUE_NIL,
  VALUE_INTEGER,
  VALUE_NUMBER,
  VALUE_STRING,
  VALUE_PRIMITIVE_ARRAY,
};

struct value {
  enum value_types type;
  union {
    vm_int integer;
    vm_number number;
    struct string_gcobject* string;
    struct array_primitive_gcobject* primitiveArray;
  } data;
};

static inline struct value value_new_none(struct vm* vm);
static inline struct value value_new_nil(struct vm* vm);
static inline struct value value_new_int(struct vm* vm, vm_int integer);
static inline struct value value_new_number(struct vm* vm, vm_number integer);

static inline struct value value_new_cstring(struct vm* vm, const char* str);
static inline struct value value_new_string(struct vm* vm, const char* str, size_t len);
static inline struct value value_new_primitive_array(struct vm* vm, size_t len);

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
bool value_is_byref(struct vm* vm, struct value a);
bool value_is_byval(struct vm* vm, struct value a);

// NULL if type not appropriate
fluffygc_object* value_get_gcobject(struct vm* vm, struct value val);
void value_set_gcobject(struct vm* vm, struct value* val, fluffygc_object* obj);

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

static inline struct value value_new_cstring(struct vm* F, const char* str) {
  return value_new_string(F, str, strlen(str));
}

static inline struct value value_new_string(struct vm* F, const char* str, size_t len) {
  return (struct value) {
    .type = VALUE_STRING,
    .data.string = string_new_string(F, str, len)
  };
}

static inline struct value value_new_primitive_array(struct vm* F, size_t len) {
  return (struct value) {
    .type = VALUE_PRIMITIVE_ARRAY,
    .data.primitiveArray = array_primitive_new(F, len)
  };
}

#endif

