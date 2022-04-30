#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <Block.h>
#include <inttypes.h>

#include "value.h"
#include "fluffyvm.h"
#include "foxgc.h"
#include "fluffyvm_types.h"
#include "config.h"
#include "hashtable.h"

#ifdef FLUFFYVM_HASH_USE_XXHASH
# include <xxhash.h>
#endif

static struct value valueNotPresent = {
  .type = FLUFFYVM_NOT_PRESENT,
  .data = {0}
};

static struct value valueNil = {
  .type = FLUFFYVM_TVALUE_NIL,
  .data = {0}
}; 

#define new_static_string(name, string) do { \
  foxgc_root_reference_t* tmpRef;\
  struct value tmp = value_new_string(vm, (string), &tmpRef); \
  value_copy(&vm->valueStaticData->name, &tmp); \
  foxgc_api_root_add(vm->heap, value_get_object_ptr(tmp), vm->staticDataRoot, &vm->valueStaticData->name ## RootRef); \
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmpRef); \
} while (0)

#define GET_STATIC(vm, name) (vm->valueStaticData->name)

void value_init(struct fluffyvm* vm) {
  vm->valueStaticData = malloc(sizeof(*vm->valueStaticData));
  new_static_string(outOfMemoryString, "out of memory");
  
  new_static_string(typenames.nil, "nil");
  new_static_string(typenames.string, "string");
  new_static_string(typenames.doubleNum, "double");
  new_static_string(typenames.longNum, "long");
  new_static_string(typenames.table, "table");
}

void value_cleanup(struct fluffyvm* vm) {
  foxgc_api_remove_from_root2(vm->heap, vm->staticDataRoot, GET_STATIC(vm, outOfMemoryStringRootRef));
  
  foxgc_api_remove_from_root2(vm->heap, vm->staticDataRoot, GET_STATIC(vm, typenames.nilRootRef));
  foxgc_api_remove_from_root2(vm->heap, vm->staticDataRoot, GET_STATIC(vm, typenames.stringRootRef));
  foxgc_api_remove_from_root2(vm->heap, vm->staticDataRoot, GET_STATIC(vm, typenames.longNumRootRef));
  foxgc_api_remove_from_root2(vm->heap, vm->staticDataRoot, GET_STATIC(vm, typenames.doubleNumRootRef));
  foxgc_api_remove_from_root2(vm->heap, vm->staticDataRoot, GET_STATIC(vm, typenames.tableRootRef));

  free(vm->valueStaticData);
}

static void commonStringInit(struct value_string* str, foxgc_object_t* strObj) {
  str->hashCode = 0;
  str->str = strObj;
}

static struct value value_new_string2(struct fluffyvm* vm, const char* str, size_t len, foxgc_root_reference_t** rootRef) {
  struct value_string* strStruct = malloc(sizeof(*strStruct));
  foxgc_object_t* strObj = foxgc_api_new_data_array(vm->heap, fluffyvm_get_root(vm), rootRef, 1, len, Block_copy(^void (foxgc_object_t* obj) {
    free(strStruct);
  }));

  if (!strObj) {
    free(strStruct);
    return valueNotPresent;
  }

  struct value value = {
    .data.str = strStruct,
    .type = FLUFFYVM_TVALUE_STRING
  };

  commonStringInit(strStruct, strObj);
   
  // memcpy because the string could have embedded
  // null to keep lua compat for the string
  memcpy(foxgc_api_object_get_data(strObj), str, len);
  return value;
}

struct value value_new_string(struct fluffyvm* vm, const char* cstr, foxgc_root_reference_t** rootRef) {
  return value_new_string2(vm, cstr, strlen(cstr) + 1, rootRef);
}

struct value value_new_long(struct fluffyvm* vm, int64_t integer) {
  struct value value = {
   .data.longNum = integer,
   .type = FLUFFYVM_TVALUE_LONG
  };

  return value;
}

struct value value_new_table(struct fluffyvm* vm, int loadFactor, int initialCapacity, foxgc_root_reference_t** rootRef) {
  struct hashtable* hashtable = hashtable_new(vm, loadFactor, initialCapacity, fluffyvm_get_root(vm), rootRef);

  if (!hashtable)
    return valueNotPresent;
  
  struct value value = {
    .data.table = hashtable->gc_this,
    .type = FLUFFYVM_TVALUE_TABLE
  };

  return value;
}

foxgc_object_t* value_get_object_ptr(struct value value) {
  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      return value.data.str->str;
    case FLUFFYVM_TVALUE_TABLE:
      return value.data.table;
    
    case FLUFFYVM_TVALUE_LONG:
    case FLUFFYVM_TVALUE_DOUBLE:
    case FLUFFYVM_TVALUE_NIL:
      return NULL;
    
    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
  }
}

#ifdef FLUFFYVM_HASH_USE_OPENJDK8
// OpenJDK 8 hash
static uintptr_t do_hash(void* data, size_t len) {
  uintptr_t hash = 0;
  for (int i = 0; i < len; i++)
    hash = 31 * hash + ((uint8_t*) data)[i];

  return hash;
}
#endif

#ifdef FLUFFYVM_HASH_USE_XXHASH
// xxHash
static uintptr_t do_hash(void* data, size_t len) {
  return (uintptr_t) XXH64(data, len, FLUFFYVM_XXHASH_SEED);
}
#endif

#ifdef FLUFFYVM_HASH_USE_FNV
static uintptr_t do_hash(void* data, size_t len) {
  uintptr_t hash = FLUFFYVM_FNV_OFFSET_BASIS;

  for (int i = 0; i < len; i++) {
    hash = hash ^ ((uint8_t*) data)[i];
    hash = hash * FLUFFYVM_FNV_PRIME;
  }

  return hash;
}
#endif

#ifdef FLUFFYVM_HASH_USE_TEST_MODE
static uintptr_t do_hash(void* data, size_t len) {
  uintptr_t hash = 0;
  for (int i = 0; i < len && i < sizeof(uintptr_t); i++) {
    hash |= ((uintptr_t) ((uint8_t*) data)[i]) << (i * 8);
  }
  return hash;
} 
#endif

bool value_hash_code(struct value value, uintptr_t* hashCode) {
  // Compute hash code
  uintptr_t hash = 0;
  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      if (value.data.str->hashCode != 0) {
        hash = value.data.str->hashCode;
        break;
      }
      
      void* data = foxgc_api_object_get_data(value.data.str->str);
      size_t len = foxgc_api_get_array_length(value.data.str->str);

      hash = do_hash(data, len);
      value.data.str->hashCode = hash;
      break;
    
    case FLUFFYVM_TVALUE_LONG:
      hash = value.data.longNum;
      break;
    case FLUFFYVM_TVALUE_DOUBLE:
    case FLUFFYVM_TVALUE_NIL:
      break;
    
    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
    case FLUFFYVM_TVALUE_TABLE:
      hash = do_hash((void*) (&value.data.table), sizeof(foxgc_object_t*));
      break;
  }
  
  *hashCode = hash;
  return true;
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

struct value value_tostring(struct fluffyvm* vm, struct value value, struct value* errorMessage, foxgc_root_reference_t** rootRef) {
  checkPresent(&value);

  size_t bufLen = 0;
  char* buffer = NULL;
  
  // Get the len
  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      foxgc_api_root_add(vm->heap, value.data.str->str, fluffyvm_get_root(vm), rootRef);
      return value;
    
    case FLUFFYVM_TVALUE_LONG:
      bufLen = snprintf(NULL, 0, "%ld", value.data.longNum);
      break;

    case FLUFFYVM_TVALUE_DOUBLE:
      bufLen = snprintf(NULL, 0, "%lf", value.data.doubleData);
      break;

    case FLUFFYVM_TVALUE_NIL:
      return vm->valueStaticData->typenames.nil;
    
    case FLUFFYVM_TVALUE_TABLE:
      // I heard %p is GNU extension
      bufLen = snprintf(NULL, 0, "table 0x%" PRIXPTR, (uintptr_t) value.data.table);
      break;

    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
  }
  
  struct value_string* strStruct = malloc(sizeof(*strStruct));
  foxgc_object_t* obj = foxgc_api_new_data_array(vm->heap, fluffyvm_get_root(vm), rootRef, 1, bufLen, Block_copy(^void (foxgc_object_t* obj) {
    free(strStruct); 
  }));

  if (obj == NULL)
    goto no_memory;
  buffer = foxgc_api_object_get_data(obj);
  
  struct value result = {
    .type = FLUFFYVM_TVALUE_STRING,
    .data.str = strStruct
  };
  commonStringInit(strStruct, obj);
   
  switch (value.type) { 
    case FLUFFYVM_TVALUE_LONG:
      sprintf(buffer, "%ld", value.data.longNum);
      break;

    case FLUFFYVM_TVALUE_DOUBLE:
      sprintf(buffer, "%lf", value.data.doubleData);
      break;
    
    case FLUFFYVM_TVALUE_TABLE:
      bufLen = sprintf(buffer, "table 0x%" PRIXPTR, (uintptr_t) value.data.table);
      break;

    case FLUFFYVM_TVALUE_STRING:
    case FLUFFYVM_TVALUE_NIL:  
    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
  }
  return result;
  
  no_memory:
  free(strStruct);
  if (errorMessage) {
    value_copy(errorMessage, &vm->valueStaticData->outOfMemoryString);
    foxgc_object_t* ptr;
    if ((ptr = value_get_object_ptr(*errorMessage))) 
      foxgc_api_root_add(vm->heap, ptr, fluffyvm_get_root(vm), rootRef);
  }
  return valueNotPresent;
}

struct value value_typename(struct fluffyvm* vm, struct value value) {
  return valueNotPresent;
}

struct value value_todouble(struct fluffyvm* vm, struct value value, struct value* errorMessage) {
  checkPresent(&value);
  
  char* lastChar = NULL;
  double number = 0.0f;
  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      errno = 0;
      number = strtod(foxgc_api_object_get_data(value.data.str->str), &lastChar);
      if (*lastChar != '\0' || errno != 0) {
        return valueNil;
      }
      break;

    case FLUFFYVM_TVALUE_LONG:
      number = (double) value.data.longNum;
      break;

    case FLUFFYVM_TVALUE_DOUBLE:
      return value;

    case FLUFFYVM_TVALUE_TABLE:
      return valueNotPresent;

    case FLUFFYVM_TVALUE_NIL:
      return valueNil;
    
    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
  }

  return value_new_double(vm, number);
}


void* value_get_unique_ptr(struct value value) {
  checkPresent(&value);

  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      return value.data.str;
    case FLUFFYVM_TVALUE_TABLE:
      return value.data.table;
    
    case FLUFFYVM_TVALUE_LONG:
    case FLUFFYVM_TVALUE_DOUBLE:
    case FLUFFYVM_TVALUE_NIL:
      return NULL;
    
    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
  }
}

bool value_equals(struct value op1, struct value op2) {
  bool result = false;
  uintptr_t op1Hash;
  uintptr_t op2Hash;
  
  checkPresent(&op1);
  checkPresent(&op2);

  if (op1.type != op2.type)
    return false;

  switch (op1.type) {
    case FLUFFYVM_TVALUE_STRING:
      if (value_get_len(op1) != value_get_len(op2))
        break;
      value_hash_code(op1, &op1Hash);
      value_hash_code(op2, &op2Hash);

      if (op1Hash == op2Hash)
        result = memcmp(value_get_string(op1), value_get_string(op2), value_get_len(op1)) == 0;
      break;
    case FLUFFYVM_TVALUE_TABLE:
      result = op1.data.table == op2.data.table;
      break;
    case FLUFFYVM_TVALUE_LONG:
      result = op1.data.longNum == op2.data.longNum;
      break;
    case FLUFFYVM_TVALUE_DOUBLE:
      result = op1.data.doubleData == op2.data.doubleData;
      break;
    case FLUFFYVM_TVALUE_NIL:
      return true;
    
    case FLUFFYVM_NOT_PRESENT:
      abort(); /* Can't happen */
  }

  return result;
}

bool value_table_set(struct fluffyvm* vm, struct value table, struct value key, struct value value) {
  checkPresent(&table);
  checkPresent(&key);
  checkPresent(&value);
  if (table.type != FLUFFYVM_TVALUE_TABLE)
    return false;
  
  hashtable_set(vm, foxgc_api_object_get_data(table.data.table), key, value);
  return true;
}
struct value value_table_get(struct fluffyvm* vm, struct value table, struct value key, foxgc_root_reference_t** rootRef) {
  checkPresent(&table);
  checkPresent(&key);
  if (table.type != FLUFFYVM_TVALUE_TABLE)
    return valueNotPresent;
  
  return hashtable_get(vm, foxgc_api_object_get_data(table.data.table), key, rootRef);
}

struct value value_not_present() {
  return valueNotPresent;
}
struct value vdalue_nil() {
  return valueNil;
}

