#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"
#include "fluffyvm.h"
#include "foxgc.h"
#include "ref_counter.h"
#include "fluffyvm_types.h"

#define value_copy(dest, src) do { \
  memcpy((dest), (src), sizeof(struct value)); \
} while(0);

static struct value valueNotPresent = {
  .type = FLUFFYVM_NOT_PRESENT,
  .data = {0}
};

#define value_new_static_string(name, string) do { \
  struct value tmp = value_new_string(vm, (string)); \
  value_copy(&vm->valueStaticData->name, &tmp); \
} while (0)

void value_init(struct fluffyvm* vm) {
  value_new_static_string(nilString, "nil");
  value_new_static_string(outOfMemoryString, "out of memory");
}

void value_cleanup(struct fluffyvm* vm) {
  value_try_decrement_ref(vm->valueStaticData->nilString);
  value_try_decrement_ref(vm->valueStaticData->outOfMemoryString);
}

struct value value_new_string2(struct fluffyvm* vm, const char* str, size_t len) {
  foxgc_root_reference_t* rootRef = NULL;
  foxgc_object_t* strObj = foxgc_api_new_data_array(vm->heap, fluffyvm_get_root(vm), &rootRef, 1, len);
  if (!strObj) {
    return valueNotPresent;
  }

  struct value value = {
    .data.str = strObj,
    .type = FLUFFYVM_TVALUE_STRING
  };

  memcpy(foxgc_api_object_get_data(value.data.str), str, len);
  
  foxgc_api_increment_ref(strObj);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), rootRef);
  return value;
}

struct value value_new_string(struct fluffyvm* vm, const char* cstr) {
  return value_new_string2(vm, cstr, strlen(cstr) + 1);
}

struct value value_new_integer(struct fluffyvm* vm, int integer) {
  struct value value = {
   .data.integer = integer,
   .type = FLUFFYVM_TVALUE_INTEGER
  };

  return value;
}

struct value value_new_double(struct fluffyvm* vm, double number) {
  struct value value = {
    .data.doubleData = number,
    .type = FLUFFYVM_TVALUE_DOUBLE
  };

  return value;
}

static void checkPresent(struct value* value) {
  if (value->type == FLUFFYVM_NOT_PRESENT)
    abort();
}

void value_try_increment_ref(struct value value) {
  checkPresent(&value);

  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING: 
      foxgc_api_increment_ref(value.data.str);
      break;
    
    case FLUFFYVM_TVALUE_INTEGER:
    case FLUFFYVM_TVALUE_DOUBLE:
    case FLUFFYVM_TVALUE_NIL:
      break;
    
    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
  }
}

void value_try_decrement_ref(struct value value) {
  checkPresent(&value);

  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      foxgc_api_decrement_ref(value.data.str);
      break;
    
    case FLUFFYVM_TVALUE_INTEGER:
    case FLUFFYVM_TVALUE_DOUBLE:
    case FLUFFYVM_TVALUE_NIL:
      break;
    
    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
  }
}

struct value value_tostring(struct fluffyvm* vm, struct value value, struct value* errorMessage) {
  checkPresent(&value);

  size_t bufLen = 0;
  char* buffer = NULL;
  
  // Get the len
  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      return value;
    
    case FLUFFYVM_TVALUE_INTEGER:
      bufLen = snprintf(NULL, 0, "%d", value.data.integer);
      break;

    case FLUFFYVM_TVALUE_DOUBLE:
      bufLen = snprintf(NULL, 0, "%lf", value.data.doubleData);
      break;

    case FLUFFYVM_TVALUE_NIL:
      return vm->valueStaticData->nilString;
    
    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
  }
  
  foxgc_root_reference_t* rootRef = NULL;
  struct value result = {
    .type = FLUFFYVM_TVALUE_STRING,
    .data.str = foxgc_api_new_data_array(vm->heap, fluffyvm_get_root(vm), &rootRef, 1, bufLen)
  };
  
  if (result.data.str == NULL)
    goto no_memory;
  buffer = foxgc_api_object_get_data(result.data.str);

  switch (value.type) { 
    case FLUFFYVM_TVALUE_INTEGER:
      sprintf(buffer, "%d", value.data.integer);
      break;

    case FLUFFYVM_TVALUE_DOUBLE:
      sprintf(buffer, "%lf", value.data.doubleData);
      break;

    case FLUFFYVM_TVALUE_STRING:
    case FLUFFYVM_TVALUE_NIL:  
    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
  }
  
  foxgc_api_increment_ref(result.data.str);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), rootRef);
  return result;
  
  no_memory:
  value_copy(errorMessage, &vm->valueStaticData->outOfMemoryString);
  value_try_increment_ref(vm->valueStaticData->outOfMemoryString);
  return valueNotPresent;
}

struct value value_tonumber(struct fluffyvm* vm, struct value value, struct value* errorMessage) {
  checkPresent(&value);
  
  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      return valueNotPresent;
    
    case FLUFFYVM_TVALUE_INTEGER:
    case FLUFFYVM_TVALUE_DOUBLE:
    case FLUFFYVM_TVALUE_NIL:
      break;
    
    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
  }
}


void* value_get_unique_ptr(struct value value) {
  checkPresent(&value);

  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      return value.data.str;
    
    case FLUFFYVM_TVALUE_INTEGER:
    case FLUFFYVM_TVALUE_DOUBLE:
    case FLUFFYVM_TVALUE_NIL:
      break;
    
    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
  }

  return NULL;
}



