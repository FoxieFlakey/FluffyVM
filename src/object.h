#ifndef _headers_1668320913_FluffyVM_object
#define _headers_1668320913_FluffyVM_object

// Will be root of inheritance for future Java like inheritance system 
// but for now boilerplates

#include <FluffyGC/v1.h>

// Class for this is lua.Object
struct object {
  fluffygc_object* gc_this;
  
  // Actual data here
  fluffygc_object* data;
};



#endif

