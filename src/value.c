#include "value.h"
#include "vm.h"

struct value value_new_none(struct vm* vm) {
  struct value tmp = {
    .type = VALUE_NONE
  };
  return tmp;
}

struct value value_new_nil(struct vm* vm) {
  struct value tmp = {
    .type = VALUE_NIL
  };
  return tmp;
}

struct value value_new_int(struct vm* vm, vm_int integer) {
  struct value tmp = {
    .type = VALUE_INTEGER,
    .data.integer = integer
  };
  return tmp;
}


