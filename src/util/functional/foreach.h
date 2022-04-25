#ifndef _headers_1642664060_MinecraftLauncherCLI_v2_parallel_foreach
#define _headers_1642664060_MinecraftLauncherCLI_v2_parallel_foreach

#include "functional.h"

// If threads is zero it detects how many cores the CPU support
typedef enum {
  FOREACH_OK,
  
  // Consumer return false
  FOREACH_FAILED, 
  
  // Invalid argument passed to foreach function
  FOREACH_INVALID
} foreach_status_t;

// supplier is guarantee never be called more than once at any given moment  
foreach_status_t foreach_parallel_do(supplier_t supplier, consumer_t consumer, int threads);
foreach_status_t foreach_do(supplier_t supplier, consumer_t consumer);
foreach_status_t foreach_do_list_direction(list_t* list, list_direction_t dir, consumer_t consumer);
foreach_status_t foreach_do_list(list_t* list, consumer_t consumer);

#endif

