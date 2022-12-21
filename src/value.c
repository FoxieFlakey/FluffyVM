#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>

#include "value.h"
#include "bug.h"
#include "vm.h"
#include "vm_types.h"

static int valueFixTypeToSameType(struct vm* vm, struct value* a, struct value* b) {
  if ((a->type != VALUE_INTEGER && a->type != VALUE_NUMBER) ||
      (a->type != VALUE_INTEGER && a->type != VALUE_NUMBER))
    return -EINVAL;

  if (a->type == b->type)
    return a->type;

  if (a->type == VALUE_INTEGER)
    *a = value_new_number(vm, a->data.integer);
  else  
    *b = value_new_number(vm, b->data.integer);

  return a->type;
}

fluffygc_object* value_get_gcobject(struct vm* vm, struct value val) {
  switch (val.type) { 
    case VALUE_STRING: 
      return cast_to_gcobj(val.data.string);
    case VALUE_PRIMITIVE_ARRAY: 
      return cast_to_gcobj(val.data.primitiveArray); 
    default: 
      return NULL; 
  } 
}

void value_set_gcobject(struct vm* vm, struct value* val, fluffygc_object* obj) {
  switch (val->type) { 
    case VALUE_STRING: 
      val->data.string = NULL;
    case VALUE_PRIMITIVE_ARRAY: 
      val->data.primitiveArray = NULL; 
    default: 
      abort(); 
  } 
}

bool value_is_byref(struct vm* vm, struct value val) {
  return value_get_gcobject(vm, val) != NULL;
}

bool value_is_byval(struct vm* vm, struct value val) {
  return value_get_gcobject(vm, val) == NULL;
}



#define mathOp(name) \
  int name(struct vm* vm, struct value* res, struct value a, struct value b) { \
    int finalType = valueFixTypeToSameType(vm, &a, &b); \
    if (finalType < 0) { \
      /* TODO: Planned for metaevent trigger */ \
      return -EFAULT; \
    } \
     \
    switch (finalType) { \
      case VALUE_INTEGER: \
        *res = value_new_int(vm, name ## _operation_integer(a.data.integer, b.data.integer)); \
        break; \
      case VALUE_NUMBER: \
        *res = value_new_number(vm, name ## _operation_number(a.data.number, b.data.number)); \
        break; \
      default: \
        abort(); \
    } \
    return 0; \
  }

#define value_add_operation_integer(a, b) a + b
#define value_add_operation_number(a, b) a + b
mathOp(value_add);

#define value_sub_operation_integer(a, b) a - b
#define value_sub_operation_number(a, b) a - b
mathOp(value_sub);

#define value_mul_operation_integer(a, b) a * b
#define value_mul_operation_number(a, b) a * b
mathOp(value_mul);

#define value_div_operation_integer(a, b) a / b
#define value_div_operation_number(a, b) a / b
mathOp(value_div);

#define value_mod_operation_integer(a, b) a % b
#define value_mod_operation_number(a, b) ((vm_number) fmodl(a, b))
mathOp(value_mod);

#define value_pow_operation_integer(a, b) ((vm_int) powl(a, b))
#define value_pow_operation_number(a, b) ((vm_number) powl(a, b))
mathOp(value_pow);

bool value_is_less(struct vm* vm, struct value a, struct value b) {
  return a.data.integer < b.data.integer;
}

bool value_is_equal(struct vm* vm, struct value a, struct value b) {
  return a.data.integer == b.data.integer;
}


