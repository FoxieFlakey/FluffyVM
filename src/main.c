#include <stdio.h>

#include "foxgc.h"
#include "ref_counter.h"
#include "fluffyvm.h"
#include "value.h"

#define KB (1024)
#define MB (1024 * KB)

int main() {
  foxgc_heap_t* heap = foxgc_api_new(2 * MB, 4 * MB, 16 * MB,
                                 5, 5, 

                                 8 * KB, 1 * MB,

                                 32 * KB);
  struct fluffyvm* F = fluffyvm_new(heap);
  
  struct value val = {};
  if (!value_new_string(F, &val, "Hello World!"))
    goto no_memory;
  
  foxgc_api_do_full_gc(heap);
  
  puts(foxgc_api_object_get_data(val.data.str));
  value_try_decrement_ref(val);
  
  foxgc_api_do_full_gc(heap);
  
  no_memory:
  fluffyvm_free(F);
  foxgc_api_free(heap);
  puts("Exiting :3");
}



