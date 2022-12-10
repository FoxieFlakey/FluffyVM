#ifndef _headers_1668313795_FluffyVM_value_container
#define _headers_1668313795_FluffyVM_value_container

// Contain struct value in GC object
#include <FluffyGC/v1.h>

#include "value.h"

struct value_container {
  fluffygc_object* gc_this;
  
  struct value data;
  fluffygc_object* obj;
};

struct value_container_gcobject {};

#endif

