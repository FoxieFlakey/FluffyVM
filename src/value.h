#ifndef header_1650698040_value_h
#define header_1650698040_value_h

#include "fluffyvm.h"
#include "ref_counter.h"

typedef enum value_types {
  // Call abort when trying to use value
  // with this type
  FLUFFYVM_NOT_PRESENT,

  FLUFFYVM_TVALUE_NIL,
  FLUFFYVM_TVALUE_DOUBLE,
  FLUFFYVM_TVALUE_INTEGER,
  FLUFFYVM_TVALUE_STRING
} value_types_t;

typedef struct value {
  const value_types_t type;
  
  const union {
    // const char*
    foxgc_object_t* str;

    double doubleData;
    int integer;
  } data;
} value_t;

void value_init(struct fluffyvm* vm);
void value_cleanup(struct fluffyvm* vm);

struct value value_new_string(struct fluffyvm* vm, const char* cstr);
struct value value_new_integer(struct fluffyvm* vm, int integer);
struct value value_new_double(struct fluffyvm* vm, double number);

// Don't access the pointer
// Return NULL if its not by reference
void* value_get_unique_ptr(struct value value);

// Try increment/decrement ref count
// on value that is by reference like
// strings, tables, closures, etc
void value_try_increment_ref(struct value value);
void value_try_decrement_ref(struct value value);

// Action you can do with value
// return value with type of FLUFFYVM_NOT_PRESENT
// if error occured
// and set *errorMessage to reason
struct value value_tostring(struct fluffyvm* vm, struct value value, struct value* errorMessage);
struct value value_tonumber(struct fluffyvm* vm, struct value value, struct value* errorMessage);

#endif






