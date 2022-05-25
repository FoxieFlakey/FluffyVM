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
#include "closure.h"

#ifdef FLUFFYVM_HASH_USE_XXHASH
# include <xxhash.h>
#endif

static struct value valueNotPresent = {
  .type = FLUFFYVM_TVALUE_NOT_PRESENT,
  .data = {0}
};

static struct value valueNil = {
  .type = FLUFFYVM_TVALUE_NIL,
  .data = {0}
}; 

bool value_init(struct fluffyvm* vm) {
  return true;
}

void value_cleanup(struct fluffyvm* vm) {
}

static void commonStringInit(struct value_string* str, foxgc_object_t* strObj) {
  str->hashCode = 0;
  str->str = strObj;
}

struct value value_new_string2(struct fluffyvm* vm, const char* str, size_t len, foxgc_root_reference_t** rootRef) {
  struct value_string* strStruct = malloc(sizeof(*strStruct));
  if (!strStruct) {
    if (vm->staticStrings.outOfMemoryRootRef)
      fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return valueNotPresent;
  }

  foxgc_object_t* strObj = foxgc_api_new_data_array(vm->heap, fluffyvm_get_root(vm), rootRef, 1, len, Block_copy(^void (foxgc_object_t* obj) {
    free(strStruct);
  }));

  if (!strObj) {
    if (vm->staticStrings.outOfMemoryRootRef)
      fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    free(strStruct);
    return valueNotPresent;
  }

  struct value value = {
    .data.str = strStruct,
    .type = FLUFFYVM_TVALUE_STRING
  };

  commonStringInit(strStruct, strObj);
   
  // memcpy because the string could have embedded
  // null to mimic lua for the string
  memcpy(foxgc_api_object_get_data(strObj), str, len);
  return value;
}

struct value value_new_string(struct fluffyvm* vm, const char* cstr, foxgc_root_reference_t** rootRef) {
  return value_new_string2(vm, cstr, strlen(cstr), rootRef);
}

struct value value_new_long(struct fluffyvm* vm, int64_t integer) {
  struct value value = {
   .data.longNum = integer,
   .type = FLUFFYVM_TVALUE_LONG
  };

  return value;
}

struct value value_new_closure(struct fluffyvm* vm, struct fluffyvm_closure* closure) {
  struct value value = {
   .data.closure = closure,
   .type = FLUFFYVM_TVALUE_CLOSURE
  };

  return value;
}

struct value value_new_table(struct fluffyvm* vm, int loadFactor, int initialCapacity, foxgc_root_reference_t** rootRef) {
  struct hashtable* hashtable = hashtable_new(vm, loadFactor, initialCapacity, fluffyvm_get_root(vm), rootRef);

  if (!hashtable) {
    if (vm->staticStrings.outOfMemoryRootRef)
      fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return valueNotPresent;
  }

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
    case FLUFFYVM_TVALUE_CLOSURE:
      return value.data.closure->gc_this;

    case FLUFFYVM_TVALUE_LONG:
    case FLUFFYVM_TVALUE_DOUBLE:
    case FLUFFYVM_TVALUE_NIL:
      return NULL;
    
    case FLUFFYVM_TVALUE_LAST:    
    case FLUFFYVM_TVALUE_NOT_PRESENT:
      abort(); /* Can't happen */
  }
}

#ifdef FLUFFYVM_HASH_USE_OPENJDK8
// OpenJDK 8 hash
static uint64_t do_hash(void* data, size_t len) {
  uint64_t hash = 0;
  for (int i = 0; i < len; i++)
    hash = 31 * hash + ((uint8_t*) data)[i];

  return hash;
}
#endif

#ifdef FLUFFYVM_HASH_USE_XXHASH
// xxHash
static uint64_t do_hash(void* data, size_t len) {
  return (uint64_t) XXH64(data, len, FLUFFYVM_XXHASH_SEED);
}
#endif

#ifdef FLUFFYVM_HASH_USE_FNV
static uint64_t do_hash(void* data, size_t len) {
  uint64_t hash = FLUFFYVM_FNV_OFFSET_BASIS;

  for (int i = 0; i < len; i++) {
    hash = hash ^ ((uint8_t*) data)[i];
    hash = hash * FLUFFYVM_FNV_PRIME;
  }

  return hash;
}
#endif

#ifdef FLUFFYVM_HASH_USE_TEST_MODE
static uint64_t do_hash(void* data, size_t len) {
  uint64_t hash = 0;
  for (int i = 0; i < len && i < sizeof(uint64_t); i++) {
    hash |= ((uint64_t) ((uint8_t*) data)[i]) << (i * 8);
  }
  return hash;
} 
#endif

bool value_hash_code(struct value value, uint64_t* hashCode) {
  // Compute hash code
  uint64_t hash = 0;
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
      hash = do_hash((void*) (&value.data.longNum), sizeof(long));
      break;
    case FLUFFYVM_TVALUE_DOUBLE:
      hash = do_hash((void*) (&value.data.doubleData), sizeof(double));
      break;
    case FLUFFYVM_TVALUE_TABLE:
      hash = do_hash((void*) (&value.data.table), sizeof(foxgc_object_t*));
      break;
    case FLUFFYVM_TVALUE_CLOSURE:
      hash = do_hash((void*) (&value.data.closure->gc_this), sizeof(foxgc_object_t*));
      break;
    
    case FLUFFYVM_TVALUE_NIL:
      hash = 0;
      break;

    case FLUFFYVM_TVALUE_LAST:    
    case FLUFFYVM_TVALUE_NOT_PRESENT:
      abort(); /* Can't happen */
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
  if (value->type == FLUFFYVM_TVALUE_NOT_PRESENT ||
      value->type >= FLUFFYVM_TVALUE_LAST)
    abort();
}

const char* value_get_string(struct value value) {
  checkPresent(&value);
  if (value.type != FLUFFYVM_TVALUE_STRING) {
    printf("%d\n", value.type);
    return NULL;
  }

  return (const char*) foxgc_api_object_get_data(value.data.str->str);
}

size_t value_get_len(struct value value) {
  checkPresent(&value); 

  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      return foxgc_api_get_array_length(value.data.str->str);
    case FLUFFYVM_TVALUE_TABLE:
      return ((struct hashtable*) foxgc_api_object_get_data(value.data.table))->usage;
    default:
      return -1;
  }
}

struct value value_tostring(struct fluffyvm* vm, struct value value, foxgc_root_reference_t** rootRef) {
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
      foxgc_api_root_add(vm->heap, value_get_object_ptr(vm->staticStrings.typenames_nil), fluffyvm_get_root(vm), rootRef);
      return vm->staticStrings.typenames_nil;
    
    case FLUFFYVM_TVALUE_TABLE:
      bufLen = snprintf(NULL, 0, "table 0x%" PRIXPTR, (uintptr_t) value.data.table);
      break;
    case FLUFFYVM_TVALUE_CLOSURE:
      bufLen = snprintf(NULL, 0, "function 0x%" PRIXPTR, (uintptr_t) value.data.closure->gc_this);
      break;

    case FLUFFYVM_TVALUE_LAST:    
    case FLUFFYVM_TVALUE_NOT_PRESENT:
      abort(); /* Can't happen */
  }
  
  struct value_string* strStruct = malloc(sizeof(*strStruct));
  if (strStruct == NULL)
    goto no_memory;

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
      bufLen = snprintf(buffer, bufLen, "table 0x%" PRIXPTR, (uintptr_t) value.data.table);
      break;
    case FLUFFYVM_TVALUE_CLOSURE:
      bufLen = snprintf(buffer, bufLen, "function 0x%" PRIXPTR, (uintptr_t) value.data.closure->gc_this);
      break;

    case FLUFFYVM_TVALUE_STRING:
    case FLUFFYVM_TVALUE_NIL:  
    case FLUFFYVM_TVALUE_NOT_PRESENT:
    case FLUFFYVM_TVALUE_LAST:    
      abort(); /* Can't happen */
  }
  return result;
  
  no_memory:
  free(strStruct);
  if (vm->staticStrings.outOfMemoryRootRef)
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
  return valueNotPresent;
}

struct value value_typename(struct fluffyvm* vm, struct value value) {
  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      return vm->staticStrings.typenames_string;
    case FLUFFYVM_TVALUE_LONG:
      return vm->staticStrings.typenames_longNum;
    case FLUFFYVM_TVALUE_DOUBLE:
      return vm->staticStrings.typenames_doubleNum;
    case FLUFFYVM_TVALUE_NIL:
      return vm->staticStrings.typenames_nil;
    case FLUFFYVM_TVALUE_TABLE:
      return vm->staticStrings.typenames_table;
    case FLUFFYVM_TVALUE_CLOSURE:
      return vm->staticStrings.typenames_closure;
    case FLUFFYVM_TVALUE_LAST:    
    case FLUFFYVM_TVALUE_NOT_PRESENT:
      abort();
  };
}

struct value value_todouble(struct fluffyvm* vm, struct value value) {
  checkPresent(&value);
  
  char* lastChar = NULL;
  double number = 0.0f;
  switch (value.type) {
    case FLUFFYVM_TVALUE_STRING:
      errno = 0;
      number = strtod(foxgc_api_object_get_data(value.data.str->str), &lastChar);
      if (*lastChar != '\0') {
        fluffyvm_set_errmsg(vm, vm->staticStrings.strtodDidNotProcessAllTheData);
        return valueNotPresent;
      }

      if (errno != 0) {
        static const char* format = "strtod errored with %d (%d)";
        
        int currentErrno = errno;
        char errorMessage[256] = {0};
        int err = strerror_r(currentErrno, errorMessage, sizeof(errorMessage));

        size_t len = 0;
        char* errMsg = malloc(len = snprintf(NULL, 0, format, currentErrno, err == 0 ? errorMessage : "error converting errno to string"));
        if (!errMsg) {
          fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemoryWhileAnErrorOccured);
          return valueNotPresent;
        }
        snprintf(errMsg, len, format, currentErrno, err == 0 ? errorMessage : "error converting errno to string");
        
        foxgc_root_reference_t* ref = NULL;
        struct value errorString = value_new_string(vm, errMsg, &ref);
        if (errorString.type != FLUFFYVM_TVALUE_NOT_PRESENT) {
          fluffyvm_set_errmsg(vm, errorString);
          foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), ref);
        }

        free(errMsg);
        return valueNotPresent;
      } 
      break;

    case FLUFFYVM_TVALUE_LONG:
      number = (double) value.data.longNum;
      break;

    case FLUFFYVM_TVALUE_DOUBLE:
      return value;

    case FLUFFYVM_TVALUE_CLOSURE:
    case FLUFFYVM_TVALUE_TABLE:
    case FLUFFYVM_TVALUE_NIL:
      return valueNotPresent;
    
    case FLUFFYVM_TVALUE_LAST:    
    case FLUFFYVM_TVALUE_NOT_PRESENT:
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
    case FLUFFYVM_TVALUE_CLOSURE:
      return value.data.closure->gc_this;
    
    case FLUFFYVM_TVALUE_LONG:
    case FLUFFYVM_TVALUE_DOUBLE:
    case FLUFFYVM_TVALUE_NIL:
      return NULL;
    
    case FLUFFYVM_TVALUE_LAST:    
    case FLUFFYVM_TVALUE_NOT_PRESENT:
      abort(); /* Can't happen */
  }
}

void value_copy(struct value* dest, struct value* src) {
  //checkPresent(src);
  memcpy(dest, src, sizeof(struct value));
}

bool value_equals(struct value op1, struct value op2) {
  bool result = false;
  uint64_t op1Hash;
  uint64_t op2Hash;
    
  checkPresent(&op1);
  checkPresent(&op2);

  if (op1.type != op2.type)
    return false;
  size_t maxLength = value_get_len(op2);
  if (value_get_len(op1) > maxLength)
    maxLength = value_get_len(op1);

  switch (op1.type) {
    case FLUFFYVM_TVALUE_STRING:
      if (value_get_len(op1) != value_get_len(op2))
        break;
      value_hash_code(op1, &op1Hash);
      value_hash_code(op2, &op2Hash);

      if (op1Hash == op2Hash)
        result = memcmp(value_get_string(op1), value_get_string(op2), maxLength) == 0;
      break;
    case FLUFFYVM_TVALUE_TABLE:
      result = op1.data.table == op2.data.table;
      break;
    case FLUFFYVM_TVALUE_CLOSURE:
      result = op1.data.closure == op2.data.closure;
      break;
    case FLUFFYVM_TVALUE_LONG:
      result = op1.data.longNum == op2.data.longNum;
      break;
    case FLUFFYVM_TVALUE_DOUBLE:
      result = op1.data.doubleData == op2.data.doubleData;
      break;
    case FLUFFYVM_TVALUE_NIL:
      return true;
    
    case FLUFFYVM_TVALUE_LAST:    
    case FLUFFYVM_TVALUE_NOT_PRESENT:
      abort(); /* Can't happen */
  }

  return result;
}

bool value_table_set(struct fluffyvm* vm, struct value table, struct value key, struct value value) {
  checkPresent(&table);
  checkPresent(&key);
  checkPresent(&value);
  if (table.type != FLUFFYVM_TVALUE_TABLE) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.attemptToIndexNonIndexableValue);
    return false;
  }
  
  hashtable_set(vm, foxgc_api_object_get_data(table.data.table), key, value);
  return true;
}
struct value value_table_get(struct fluffyvm* vm, struct value table, struct value key, foxgc_root_reference_t** rootRef) {
  checkPresent(&table);
  checkPresent(&key);
  if (table.type != FLUFFYVM_TVALUE_TABLE) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.attemptToIndexNonIndexableValue);
    return value_not_present();
  }
  
  return hashtable_get(vm, foxgc_api_object_get_data(table.data.table), key, rootRef);
}

struct value value_not_present() {
  return valueNotPresent;
}
struct value value_nil() {
  return valueNil;
}

bool value_table_is_indexable(struct value val) {
  switch (val.type) {
    case FLUFFYVM_TVALUE_STRING:
    case FLUFFYVM_TVALUE_DOUBLE:
    case FLUFFYVM_TVALUE_LONG:
    case FLUFFYVM_TVALUE_CLOSURE:
    case FLUFFYVM_TVALUE_NIL:
      return false;
    
    case FLUFFYVM_TVALUE_TABLE:
      return true;

    case FLUFFYVM_TVALUE_LAST:
    case FLUFFYVM_TVALUE_NOT_PRESENT:
      abort();
  }
  abort();
}

bool value_is_callable(struct value val) {
  switch (val.type) {
    case FLUFFYVM_TVALUE_STRING:
    case FLUFFYVM_TVALUE_DOUBLE:
    case FLUFFYVM_TVALUE_LONG:
    case FLUFFYVM_TVALUE_TABLE:
    case FLUFFYVM_TVALUE_NIL:
      return false;
    
    case FLUFFYVM_TVALUE_CLOSURE:
      return true;

    case FLUFFYVM_TVALUE_LAST:
    case FLUFFYVM_TVALUE_NOT_PRESENT:
      abort();
  }
  abort();
}

