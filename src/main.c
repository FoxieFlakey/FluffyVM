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

int main() {
  heap = foxgc_api_new(2 * MB, 4 * MB, 16 * MB,
                                 5, 5, 

                                 8 * KB, 1 * MB,

                                 32 * KB);
  printMemUsage("Before VM creation");

  struct fluffyvm* F = fluffyvm_new(heap);
  
  foxgc_api_do_full_gc(heap);
  foxgc_api_do_full_gc(heap);
  printMemUsage("After VM creation but before test");
  
  int test = 3;

  // Testing tostring
  if (test == 1) {
    struct value integer = value_new_integer(F, 3892);
    
    struct value errorMessage = {0};
    struct value string = value_tostring(F, integer, &errorMessage);
    if (string.type == FLUFFYVM_NOT_PRESENT) {
      printf("Conversion error: %s\n", value_get_string(errorMessage));
      goto error;
    }

    printf("Result: '%s'\n", value_get_string(string));
    uintptr_t hash = -1;
    value_hash_code(string, &hash);
    printf("Hash result: %" PRIuPTR "\n", hash);
    value_try_decrement_ref(string);
  }

  // Testing todouble
  if (test == 2)  {
    struct value string = value_new_string(F, "3892");
    struct value number = value_todouble(F, string, NULL);
      
    printf("Result: %lf\n", number.data.doubleData);
    value_try_decrement_ref(string);
  }

  // Testing hashtable
  if (test == 3) {
    foxgc_root_reference_t* hashTableRootRef = NULL;
    struct hashtable* table = hashtable_new(F, 75, 2, &hashTableRootRef);
    assert(table);

    // Test values
    const char* pairs[][2] = {
      {"key1", "UwU"},
      {"key2", "Hello Fox"},
      {"key3", "Im fox"},
      {"key4", "Hello hash!"},
      {"jcjk", "koek"},
      {"jcjk", "UwU"},
      {"key4", "Hello hash! (rewrite)"},
      {"key5", "New key5"},
      {"Hello this working", "Im the coder fox"},
      {"greet", "Im writing lua compatible VM"},
      {"name", "FluffyVM"},
      {"aujck", "eokdjf"},
      {"eojdmc", "oekdj"},
      {"Author", "Fluffy Fox"},
      {NULL, NULL}
    };

    for (int i = 0; pairs[i][0] != NULL; i++) {
      const char** pair = pairs[i];
      struct value key = value_new_string(F, pair[0]);
      struct value value = value_new_string(F, pair[1]);
 
      hashtable_set(F, table, key, value);

      value_try_decrement_ref(key);
      value_try_decrement_ref(value);
    }
    
    foxgc_api_do_full_gc(heap);
    foxgc_api_do_full_gc(heap);
    printMemUsage("Middle of test");

    const char* keyToRead[] = {
      "jcjk",
      "name",
      "key4",
      NULL
    };

    for (int i = 0; keyToRead[i] != NULL; i++) {
      struct value key = value_new_string(F, keyToRead[i]);
      struct value result = hashtable_get(F, table, key);
      if (result.type == FLUFFYVM_TVALUE_STRING) {
        printf("table['%s'] = '%s'\n", value_get_string(key), value_get_string(result));
        value_try_decrement_ref(result);
      } else {
        printf("table['%s'] = not present\n", value_get_string(key));
      }
      value_try_decrement_ref(key);
    }
    
    foxgc_api_remove_from_root2(heap, fluffyvm_get_root(F), hashTableRootRef);
  }

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



