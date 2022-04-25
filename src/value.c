#include <string.h>

#include "value.h"
#include "fluffyvm.h"
#include "foxgc.h"
#include "ref_counter.h"

void value_init(struct fluffyvm* vm) {
  
}

void value_cleanup(struct fluffyvm* vm) {
  
}

bool value_new_string2(struct fluffyvm* vm, struct value* value, const char* str, size_t len) {
  foxgc_root_reference_t* rootRef = NULL;
  foxgc_object_t* strObj = foxgc_api_new_data_array(vm->heap, fluffyvm_get_root(vm), &rootRef, 1, len);
  if (!strObj)
    return false;
   
  value->data.str = strObj;
  value->type = FLUFFYVM_TVALUE_STRING;
  memcpy(foxgc_api_object_get_data(value->data.str), str, len);

  foxgc_api_increment_ref(strObj);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), rootRef);
  return true;
}

bool value_new_string(struct fluffyvm* vm, struct value* value, const char* cstr) {
  return value_new_string2(vm, value, cstr, strlen(cstr) + 1);
}

void value_try_increment_ref(struct value value) {
  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      foxgc_api_increment_ref(value.data.str);
      break;
    case FLUFFYVM_TVALUE_INTEGER:
    case FLUFFYVM_TVALUE_NUMBER:
    case FLUFFYVM_TVALUE_NIL:
      break;
  }
}

void value_try_decrement_ref(struct value value) {
  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      foxgc_api_decrement_ref(value.data.str);
      break;
    case FLUFFYVM_TVALUE_INTEGER:
    case FLUFFYVM_TVALUE_NUMBER:
    case FLUFFYVM_TVALUE_NIL:
      break;
  }
}

