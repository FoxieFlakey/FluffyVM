#ifndef header_1650698040_value_h
#define header_1650698040_value_h

#include "fluffyvm.h"
#include "ref_counter.h"

typedef enum value_types {
  FLUFFYVM_TVALUE_NIL,
  FLUFFYVM_TVALUE_NUMBER,
  FLUFFYVM_TVALUE_INTEGER,
  FLUFFYVM_TVALUE_STRING
} value_types_t;

typedef struct value {
  value_types_t type;
  
  struct {
    // const char*
    foxgc_object_t* str;

    double number;
    int integer;
  } data;
} value_t;

void value_init(struct fluffyvm* vm);
void value_cleanup(struct fluffyvm* vm);

bool value_new_string(struct fluffyvm* vm, struct value* value, const char* cstr);

// Try increment/decrement ref count
// on value that is by reference like
// strings, tables, closures, etc
void value_try_increment_ref(struct value value);
void value_try_decrement_ref(struct value value);

#endif

