#ifndef header_1650698040_value_h
#define header_1650698040_value_h

#include <stdint.h>
#include <string.h>

#include "fluffyvm.h"
#include "foxgc.h"

typedef enum value_types {
  // Call abort when trying to use value
  // with this type
  FLUFFYVM_NOT_PRESENT,

  FLUFFYVM_TVALUE_NIL,
  FLUFFYVM_TVALUE_DOUBLE,
  FLUFFYVM_TVALUE_LONG,
  FLUFFYVM_TVALUE_STRING,
  FLUFFYVM_TVALUE_TABLE
} value_types_t;

struct value_string {
  // Make sure hashCode size is atleast pointer
  uintptr_t hashCode;
  
  // const char*
  foxgc_object_t* str;
};

typedef struct value {
  const value_types_t type;
  
  union {
    struct value_string* str;

    // struct hashtable*
    foxgc_object_t* table;

    double doubleData;
    int64_t longNum;
  } data;
} value_t;

static inline const char* value_get_string(struct value value) {
  if (value.type != FLUFFYVM_TVALUE_STRING) 
    return NULL;
  
  return (const char*) foxgc_api_object_get_data(value.data.str->str);
}

static inline size_t value_get_len(struct value value) {
  if (value.type != FLUFFYVM_TVALUE_STRING) 
    return -1;
   
  return foxgc_api_get_array_length(value.data.str->str);
}

static inline void value_copy(struct value* dest, struct value* src) {
  memcpy(dest, src, sizeof(struct value));
}

void value_init(struct fluffyvm* vm);
void value_cleanup(struct fluffyvm* vm);

struct value value_new_string(struct fluffyvm* vm, const char* cstr, foxgc_root_reference_t** rootRef);
struct value value_new_long(struct fluffyvm* vm, int64_t integer);
struct value value_new_double(struct fluffyvm* vm, double number);
struct value value_new_table(struct fluffyvm* vm, int loadFactor, int initialCapacity, foxgc_root_reference_t** rootRef); 
struct value value_not_present();
struct value value_nil();

// Get typename guarantee to be string
struct value value_typename(struct fluffyvm* vm, struct value value);

// Don't access the pointer
// Return NULL if its not by reference
void* value_get_unique_ptr(struct value value);

foxgc_object_t* value_get_object_ptr(struct value value);

// return false if its not applicable
bool value_hash_code(struct value value, uintptr_t* hashCode);

// return false if not equal
bool value_equals(struct value op1, struct value op2);

// Action you can do with value
// return value with type of FLUFFYVM_NOT_PRESENT
// if error occured
// and set *errorMessage to reason
struct value value_tostring(struct fluffyvm* vm, struct value value, struct value* errorMessage, foxgc_root_reference_t** rootRef);
struct value value_todouble(struct fluffyvm* vm, struct value value, struct value* errorMessage);

// Action to do if this table
bool value_table_set(struct fluffyvm* vm, struct value table, struct value key, struct value value);
struct value value_table_get(struct fluffyvm* vm, struct value table, struct value key, foxgc_root_reference_t** rootRef);

#endif






