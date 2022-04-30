#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#include "foxgc.h"
#include "fluffyvm.h"
#include "value.h"
#include "hashtable.h"

#define KB (1024)
#define MB (1024 * KB)

static foxgc_heap_t* heap = NULL;

static double toKB(size_t bytes) {
  return ((double) bytes) / KB;
}

static void printMemUsage(const char* msg) {
  puts("------------------------------------");
  puts(msg);
  //puts("------------------------------------");
  printf("Heap Usage: %lf / %lf KiB\n", toKB(foxgc_api_get_heap_usage(heap)), toKB(foxgc_api_get_heap_size(heap)));
  printf("Metaspace: %lf / %lf KiB\n", toKB(foxgc_api_get_metaspace_usage(heap)), toKB(foxgc_api_get_metaspace_size(heap))); 
  puts("------------------------------------");
}

static struct value tostring(struct fluffyvm* F, struct value val, foxgc_root_reference_t** rootRef) { 
  if (val.type == FLUFFYVM_NOT_PRESENT)
    return value_not_present();

  foxgc_root_reference_t* originalValRef = *rootRef;
  struct value errorMessage = {0};
  struct value tmp = value_tostring(F, val, &errorMessage, rootRef);
   
  if (tmp.type == FLUFFYVM_NOT_PRESENT) {
    printf("Error: '%s' while converting key to string\n", value_get_string(errorMessage));
    abort();
  }

  foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), originalValRef); 

  return tmp;
}

static struct value add_to_root_and_return(struct fluffyvm* F, struct value val, foxgc_root_reference_t** rootRef) { 
  if (val.type == FLUFFYVM_NOT_PRESENT)
    return value_not_present();

  foxgc_object_t* ptr;
  if ((ptr = value_get_object_ptr(val)))
    foxgc_api_root_add(F->heap, ptr, fluffyvm_get_root(F), rootRef);

  return val;
}

int main() {
  heap = foxgc_api_new(1 * MB, 4 * MB, 16 * MB,
                                 5, 5, 

                                 8 * KB, 1 * MB,

                                 32 * KB);
  printMemUsage("Before VM creation");

  struct fluffyvm* F = fluffyvm_new(heap);
  
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  printMemUsage("After VM creation but before test");
 
  /*
  foxgc_root_reference_t* tmpRootRef;
  int test = 3;

  // Testing tostring
  if (test == 1) {
    struct value integer = value_new_long(F, 3892);
    
    struct value errorMessage = {0};
    struct value string = value_tostring(F, integer, &errorMessage, &tmpRootRef);
    if (string.type == FLUFFYVM_NOT_PRESENT) {
      printf("Conversion error: %s\n", value_get_string(errorMessage));
      goto error;
    }

    printf("Result: '%s'\n", value_get_string(string));
    uintptr_t hash = -1;
    value_hash_code(string, &hash);
    printf("Hash result: %" PRIuPTR "\n", hash);
    
    foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), tmpRootRef);
  }

  // Testing todouble
  if (test == 2)  {
    struct value string = value_new_string(F, "3892", &tmpRootRef);
    struct value number = value_todouble(F, string, NULL);
     
    printf("Result: %lf\n", number.data.doubleData);
    foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), tmpRootRef);
  }

  if (test == 3) {
    foxgc_root_reference_t* tableRootRef = NULL;
    struct value table = value_new_table(F, 75, 16, &tableRootRef);
    if (table.type == FLUFFYVM_NOT_PRESENT)
      goto no_memory;
    
    // Setting
    {
      foxgc_root_reference_t* keyRef = NULL;
      foxgc_root_reference_t* valRef = NULL;
      struct value key = 
        table;
        //value_new_string(F, "This is a key", &keyRef);
      struct value value = value_new_string(F, "This is value", &valRef);
     
      value_table_set(F, table, key, value);

      if (keyRef)
        foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), keyRef);
      foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), valRef);
    }
    
    foxgc_api_do_full_gc(heap);
    foxgc_api_do_full_gc(heap);
    printMemUsage("Middle of test");

    // Getting
    {
      foxgc_root_reference_t* keyRef = NULL;
      foxgc_root_reference_t* valRef = NULL;

      struct value key =
          //value_new_string(F, "This is a key", &keyRef);
          add_to_root_and_return(F, table, &keyRef);
      struct value keyAsString = tostring(F, key, &keyRef);
      struct value value = value_table_get(F, table, key, &valRef);
      
      if (value.type == FLUFFYVM_NOT_PRESENT) {
        printf("table[%s] = not present\n", value_get_string(keyAsString));
      } else {
        struct value valueAsString = tostring(F, value, &valRef);
        printf("table[%s] = %s '%s'\n", value_get_string(keyAsString), value_get_string(value_typename(F, value)), value_get_string(valueAsString));
        foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), valRef);
      }
 
      foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), keyRef);
    }

    foxgc_api_remove_from_root2(F->heap, fluffyvm_get_root(F), tableRootRef);
  }*/
  
  /*
  // Testing hashtable (old)
  if (test == 3) {
    struct value table = value_new_table(F, 75, 16);
    if (table.type == FLUFFYVM_NOT_PRESENT) {
      goto no_memory;
    }

    // Test values
    struct value pairs[][2] = {
      {
        value_new_string(F, "key1"), 
        value_new_string(F, "UwU")
      },
      {
        value_new_string(F, "key2"), 
        value_new_string(F, "Hello Fox")
      },
      {
        value_new_string(F, "key3"), 
        value_new_string(F, "Im fox")
      },
      {
        value_new_string(F, "key4"), 
        value_new_string(F, "Hello hash!")
      },
      {
        value_new_string(F, "jcjk"), 
        value_new_string(F, "koek")
      },
      {
        value_new_string(F, "jcjk"), 
        value_new_string(F, "UwU")
      },
      {
        value_new_string(F, "key4"), 
        value_new_string(F, "Hello hash! (rewrite)")
      },
      {
        value_new_string(F, "key5"), 
        value_new_string(F, "New key5")
      },
      {
        value_new_string(F, "Hello this working"), 
        value_new_string(F, "Im the coder fox")
      },
      {
        value_new_string(F, "greet"), 
        value_new_string(F, "Im writing lua compatible VM")
      },
      {
        value_new_string(F, "name"), 
        value_new_string(F, "FluffyVM")
      },
      {
        value_new_string(F, "aujck"), 
        value_new_string(F, "eokdjf")
      },
      {
        value_new_string(F, "eojdmc"), 
        value_new_string(F, "oekdj")
      },
      {
        value_new_string(F, "Author"),
        value_new_string(F, "Fluffy Fox")
      },
      {
        value_new_string(F, "this is table"),
        value_new_table(F, 75, 64)
      },
      {
        value_new_string(F, "this is double"),
        value_new_double(F, 3.14)
      },
      {
        value_new_string(F, "this is long"),
        value_new_long(F, 8086)
      }, 
      {
        value_new_long(F, 8888),
        value_new_string(F, "Long as key and string as value")
      },
      {
        value_new_double(F, 0.8888),
        value_new_string(F, "Double as key and string as value")
      },
      {
        value_nil(),
        value_new_string(F, "Table as key and string as value")
      },
      {value_not_present(), value_not_present()}
    };

    for (int i = 0; pairs[i][0].type != FLUFFYVM_NOT_PRESENT; i++) {
      struct value key = pairs[i][0];
      struct value value = pairs[i][1];
 
      value_table_set(F, table, key, value);
      value_try_decrement_ref(key);
      value_try_decrement_ref(value);
    }
   
    foxgc_api_do_full_gc(heap);
    foxgc_api_do_full_gc(heap);
    printMemUsage("Middle of test");

    struct value keyToRead[] = {
      value_new_string(F, "jcjk"),
      value_new_string(F, "name"),
      value_new_string(F, "key4"),
      value_new_string(F, "author"),
      value_new_string(F, "Author"),
      value_new_string(F, "this is table"),
      value_new_string(F, "i have no value"),
      value_new_string(F, "this is double"),
      value_new_string(F, "this is long"),
      value_new_long(F, 8492),
      value_new_double(F, 3.149),
      value_new_long(F, 8888),
      value_new_double(F, 0.8888),
      value_not_present()
    };

    for (int i = 0; keyToRead[i].type != FLUFFYVM_NOT_PRESENT; i++) {
      struct value key = keyToRead[i];
      struct value result = value_table_get(F, table, key);
      struct value errorMessage = value_not_present();
      struct value keyAsString = value_tostring(F, key, &errorMessage);
      
      if (errorMessage.type != FLUFFYVM_NOT_PRESENT) {
        if (errorMessage.type != FLUFFYVM_TVALUE_STRING) {
          printf("Unexpected error message type: %d\n", errorMessage.type);
        } else {
          printf("Error: %s\n", value_get_string(errorMessage));
        }
        value_try_decrement_ref(errorMessage);
        goto error;
      }
            
      if (result.type != FLUFFYVM_NOT_PRESENT) {
        struct value tmp = value_not_present();
        value_copy(&errorMessage, &tmp);
        struct value resultAsString = value_tostring(F, result, &errorMessage);

        if (errorMessage.type != FLUFFYVM_NOT_PRESENT) {
          if (errorMessage.type != FLUFFYVM_TVALUE_STRING) {
            printf("Unexpected error message type: %d\n", errorMessage.type);
          } else {
            printf("Error: %s\n", value_get_string(errorMessage));
          }
          value_try_decrement_ref(errorMessage);
          goto error;
        }

        printf("table[%s] = %s\n", value_get_string(keyAsString), value_get_string(resultAsString));
        value_try_decrement_ref(resultAsString);
      } else {
        printf("table[%s] = not present\n", value_get_string(keyAsString));
      }
      
      if (result.type != FLUFFYVM_NOT_PRESENT)
        value_try_decrement_ref(result);
      value_try_decrement_ref(keyAsString);
      value_try_decrement_ref(key);
    }
    
    value_try_decrement_ref(table);
  }
  */

  abort_test:
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  printMemUsage("Before VM destruction but after test");

  error:
  no_memory:
  fluffyvm_free(F);

  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  printMemUsage("After VM destruction");
  foxgc_api_free(heap);
  puts("Exiting :3");
}



