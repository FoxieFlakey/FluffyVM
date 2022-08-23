#ifndef header_1660463417_214133f8_ad42_4a8b_9306_246d4dd87889_value_h
#define header_1660463417_214133f8_ad42_4a8b_9306_246d4dd87889_value_h

#include "vm_types.h"

struct vm;
enum value_types {
  VALUE_NONE,
  VALUE_NIL,
  VALUE_INTEGER
};

struct value {
  enum value_types type;
  union {
    vm_int integer;
  } data;
};

struct value value_new_none(struct vm* vm);
struct value value_new_nil(struct vm* vm);
struct value value_new_int(struct vm* vm, vm_int integer);

#endif

