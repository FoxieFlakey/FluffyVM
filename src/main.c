#include <stdio.h>

#include "foxgc.h"
#include "ref_counter.h"
#include "fluffyvm.h"
#include "value.h"

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
  printMemUsage("After VM creation but before test");
  
  if (1) {
    {
      struct value integer = value_new_integer(F, 3892);
    
      struct value errorMessage = {0};
      struct value string = value_tostring(F, integer, &errorMessage);
      if (string.type == FLUFFYVM_NOT_PRESENT) {
        printf("Conversion error: %s\n", (const char*) foxgc_api_object_get_data(errorMessage.data.str));
        goto error;
      }

      value_try_decrement_ref(string);

      printf("Result: '%s'\n", (const char*) foxgc_api_object_get_data(string.data.str));
    }

    // Test 2
    {
      struct value string = value_new_string(F, "3892");
      struct value number = value_todouble(F, string, NULL);
      
      printf("Result: %lf\n", number.data.doubleData);

      value_try_decrement_ref(string);
    }
  }

  foxgc_api_do_full_gc(heap);
  printMemUsage("Before VM destruction but after test");

  error:
  no_memory:
  fluffyvm_free(F);

  foxgc_api_do_full_gc(heap);
  printMemUsage("After VM destruction");
  foxgc_api_free(heap);
  puts("Exiting :3");
}



