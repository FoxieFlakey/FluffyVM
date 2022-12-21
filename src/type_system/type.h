#ifndef _headers_1670728136_FluffyVM_type_descriptor
#define _headers_1670728136_FluffyVM_type_descriptor

#include <FluffyGC/v1.h>
#include <stdbool.h>

struct type {
  bool isPrimitive;
  fluffygc_descriptor* desc;
};

struct type* type_new(bool isPrimitive, fluffygc_descriptor* desc);


#endif

