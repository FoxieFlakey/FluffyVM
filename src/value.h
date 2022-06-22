#ifndef header_1650698040_value_h
#define header_1650698040_value_h

#include <stdint.h>
#include <stdarg.h> 
#include <string.h>
#include <stdbool.h>

#include "foxgc.h"
#include "api_layer/types.h"
#include "util/functional/functional.h"

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
  FLUFFYVM_TVALUE_BOOL,
  FLUFFYVM_TVALUE_COROUTINE,
  
  // Userdata
  FLUFFYVM_TVALUE_FULL_USERDATA,
  FLUFFYVM_TVALUE_LIGHT_USERDATA,
  FLUFFYVM_TVALUE_GARBAGE_COLLECTABLE_USERDATA,

  FLUFFYVM_TVALUE_LAST
} value_types_t;

typedef enum {
  FLUFFYVM_CALL_OK,
  FLUFFYVM_CALL_CANT_CALL,
  FLUFFYVM_CALL_RESULT_ERROR
} value_call_status_t;

typedef void (^value_userdata_finalizer)();

struct value_userdata {
  foxgc_object_t* dataObj;
  
  void* data;
  foxgc_object_t* userGarbageCollectableData;
  
  bool isFull;

  // Library or host can freely assign any
  // value for `typeID` but `moduleID` must
  // be fetch from `value_get_module_id` and
  // the module ID is only valid for entire
  // process lifetime (this make it easier
  // to decide whether a given userdata
  // is own by a module or not)
  //
  // Module ID zero is invalid in all
  // context unless noted
  int moduleID;
  int typeID;
};

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
  value_types_t type;
  
  union {
    struct value_string* str;

    // struct hashtable*
    foxgc_object_t* table;

    fluffyvm_number doubleData;
    fluffyvm_integer longNum;
    struct fluffyvm_closure* closure;

    struct value_userdata* userdata;
    bool boolean;

    struct fluffyvm_coroutine* coroutine;
  } data;
} value_t;

const char* value_get_string(struct value value);
size_t value_get_len(struct value value);

bool value_init(struct fluffyvm* vm);
void value_cleanup(struct fluffyvm* vm);

struct value value_string_allocator(struct fluffyvm* vm, const char* str, size_t len, foxgc_root_reference_t** rootRef, void* udata, runnable_t finalizer);
struct value value_new_string2(struct fluffyvm* vm, const char* str, size_t len, foxgc_root_reference_t** rootRef);
struct value value_new_string(struct fluffyvm* vm, const char* cstr, foxgc_root_reference_t** rootRef);

// Like other 2 variant but this will cache
// the string for faster future access
struct value value_new_string2_constant(struct fluffyvm* vm, const char* str, size_t len, foxgc_root_reference_t** rootRef);
struct value value_new_string_constant(struct fluffyvm* vm, const char* cstr, foxgc_root_reference_t** rootRef);

struct value value_new_long(struct fluffyvm* vm, fluffyvm_integer integer);
struct value value_new_double(struct fluffyvm* vm, fluffyvm_number number);
struct value value_new_table(struct fluffyvm* vm, double loadFactor, int initialCapacity, foxgc_root_reference_t** rootRef); 
struct value value_new_closure(struct fluffyvm* vm, struct fluffyvm_closure* closure); 
struct value value_new_full_userdata(struct fluffyvm* vm, int moduleID, int typeID, size_t size, foxgc_root_reference_t** rootRef, value_userdata_finalizer finalizer); 
struct value value_new_light_userdata(struct fluffyvm* vm, int moduleID, int typeID, void* data, foxgc_root_reference_t** rootRef, value_userdata_finalizer finalizer); 
struct value value_new_garbage_collectable_userdata(struct fluffyvm* vm, int moduleID, int typeID, foxgc_object_t* object, foxgc_root_reference_t** rootRef); 
struct value value_new_bool(struct fluffyvm* vm, bool boolean);
struct value value_new_coroutine(struct fluffyvm* vm, struct fluffyvm_closure* closure, foxgc_root_reference_t** rootRef);
struct value value_new_coroutine2(struct fluffyvm* vm, struct fluffyvm_coroutine* co);

static const struct value value_nil = {
  .type = FLUFFYVM_TVALUE_NIL,
  .data = {0}
};

static const struct value value_not_present = {
  .type = FLUFFYVM_TVALUE_NOT_PRESENT,
  .data = {0}
};

// Get typename guarantee to be string
struct value value_typename(struct fluffyvm* vm, struct value value);
struct value value_typename2(struct fluffyvm* vm, value_types_t value);

// Don't access the pointer
// Return NULL if its not by reference
void* value_get_unique_ptr(struct value value);

foxgc_object_t* value_get_object_ptr(struct value value);

// return false if its not applicable
bool value_hash_code(struct value value, uint64_t* hashCode);

// return false if not equal
bool value_equals(struct value op1, struct value op2);
bool value_equals_cstring(struct value op1, const char* str, size_t len);

bool value_is_callable(struct value val);
bool value_call(struct value val);

// Action you can do with value
// Sets errmsg on error with message of error
struct value value_tostring(struct fluffyvm* vm, struct value value, foxgc_root_reference_t** rootRef);
struct value value_todouble(struct fluffyvm* vm, struct value value);

// Action to do if this table
bool value_table_set(struct fluffyvm* vm, struct value table, struct value key, struct value value);
//bool value_table_remove(struct fluffyvm* vm, struct value table, struct value key);
struct value value_table_get(struct fluffyvm* vm, struct value table, struct value key, foxgc_root_reference_t** rootRef);
bool value_table_is_indexable(struct value val);

bool value_is_numeric(struct value val);

int value_get_module_id();

// Math operations
typedef struct value (*value_math_operation_t)(struct fluffyvm* vm, struct value op1, struct value op2);
#define VALUE_DECLARE_MATH_OP(name) struct value name(struct fluffyvm* vm, struct value op1, struct value op2)

#define VALUE_MATH_OPS \
  X(add, +) \
  X(sub, -) \
  X(mul, *) \
  X(div, /)

#define X(name, ...) VALUE_DECLARE_MATH_OP(value_math_ ## name);
VALUE_MATH_OPS
#undef X

VALUE_DECLARE_MATH_OP(value_math_pow);
VALUE_DECLARE_MATH_OP(value_math_mod);

typedef enum {
  VALUE_CMP_INAPPLICABLE,
  VALUE_CMP_FALSE,
  VALUE_CMP_TRUE
} value_comparison_result_t;

value_comparison_result_t value_is_equal(struct fluffyvm* vm, struct value op1, struct value op2);

// Only implicity convert long to double
// when needed
value_comparison_result_t value_is_less(struct fluffyvm* vm, struct value op1, struct value op2);

#endif





