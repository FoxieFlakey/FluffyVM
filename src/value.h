#ifndef header_1650698040_value_h
#define header_1650698040_value_h

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "foxgc.h"
#include "api_layer/types.h"

struct fluffyvm;
typedef enum value_types {
  // Call abort when trying to use value
  // with this type
  FLUFFYVM_TVALUE_NOT_PRESENT,

  FLUFFYVM_TVALUE_NIL,
  FLUFFYVM_TVALUE_DOUBLE,
  FLUFFYVM_TVALUE_LONG,
  FLUFFYVM_TVALUE_STRING,
  FLUFFYVM_TVALUE_TABLE,
  FLUFFYVM_TVALUE_CLOSURE,

  FLUFFYVM_TVALUE_LAST
} value_types_t;

typedef enum {
  FLUFFYVM_CALL_OK,
  FLUFFYVM_CALL_CANT_CALL,
  FLUFFYVM_CALL_RESULT_ERROR
} value_call_status_t;

struct value_string {
  // 64-bit as i was forgot that
  // uintptr_t is not guarantee 
  // to be largest type
  uint64_t hashCode;
  
  // const char*
  foxgc_object_t* str;
  bool hasNullTerminator;
};

typedef struct value {
  const value_types_t type;
  
  union {
    struct value_string* str;

    // struct hashtable*
    foxgc_object_t* table;

    fluffyvm_number doubleData;
    fluffyvm_integer longNum;
    struct fluffyvm_closure* closure;
  } data;
} value_t;

const char* value_get_string(struct value value);
size_t value_get_len(struct value value);

void value_copy(struct value* dest, struct value* src);

bool value_init(struct fluffyvm* vm);
void value_cleanup(struct fluffyvm* vm);

struct value value_new_string2(struct fluffyvm* vm, const char* str, size_t len, foxgc_root_reference_t** rootRef);
struct value value_new_string(struct fluffyvm* vm, const char* cstr, foxgc_root_reference_t** rootRef);
struct value value_new_long(struct fluffyvm* vm, fluffyvm_integer integer);
struct value value_new_double(struct fluffyvm* vm, fluffyvm_number number);
struct value value_new_table(struct fluffyvm* vm, int loadFactor, int initialCapacity, foxgc_root_reference_t** rootRef); 
struct value value_new_closure(struct fluffyvm* vm, struct fluffyvm_closure* closure); 
struct value value_not_present();
struct value value_nil();

// Get typename guarantee to be string
struct value value_typename(struct fluffyvm* vm, struct value value);

// Don't access the pointer
// Return NULL if its not by reference
void* value_get_unique_ptr(struct value value);

foxgc_object_t* value_get_object_ptr(struct value value);

// return false if its not applicable
bool value_hash_code(struct value value, uint64_t* hashCode);

// return false if not equal
bool value_equals(struct value op1, struct value op2);

bool value_is_callable(struct value val);
bool value_call(struct value val);

// Action you can do with value
// Sets errmsg on error with message of error
struct value value_tostring(struct fluffyvm* vm, struct value value, foxgc_root_reference_t** rootRef);
struct value value_todouble(struct fluffyvm* vm, struct value value);

// Action to do if this table
bool value_table_set(struct fluffyvm* vm, struct value table, struct value key, struct value value);
struct value value_table_get(struct fluffyvm* vm, struct value table, struct value key, foxgc_root_reference_t** rootRef);
bool value_table_is_indexable(struct value val);

#endif






