#include <FluffyGC/v1.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "bug.h"
#include "util.h"
#include "vm_string.h"
#include "vm.h"

static const char* stringSubystemKey = "UwU";

#define get_private(F) (F->stringData)

int string_init(struct vm* F) {
  F->stringData = malloc(sizeof(*F->stringData));
  if (!F->stringData)
    return -ENOMEM;
  
  fluffygc_field fields[] = {
    {
      .name = "actualString",
      .offset = offsetof(struct string, actualString),
      .dataType = FLUFFYGC_TYPE_NORMAL,
      .type = FLUFFYGC_FIELD_STRONG
    }
  };
  fluffygc_descriptor_args tmp = {
    .name = "net.fluffyfox.fluffyvm.String",
    .objectSize = sizeof(struct string),
    .fieldCount = ARRAY_SIZE(fields),
    .fields = fields,
    .typeID = (uintptr_t) &stringSubystemKey 
  };
  
  F->stringData->desc = fluffygc_v1_descriptor_new(F->heap, &tmp);
  if (!F->stringData->desc)
    return -ENOMEM;
  
  return 0;
}

void string_cleanup(struct vm* F) {
  if (!F->stringData)
    return;
  
  fluffygc_v1_descriptor_delete(F->heap, F->stringData->desc);
  free(F->stringData);
}

struct string_gcobject* string_new_string(struct vm* F, const char* string, size_t len) {
  fluffygc_object* self = fluffygc_v1_new_object(F->heap, get_private(F)->desc);
  if (!self)
    return NULL;
  
  fluffygc_object* stringObj = fluffygc_v1_new_opaque_object(F->heap, len + 1);
  if (!stringObj)
    goto failure;
  
  fluffygc_v1_set_object_field(F->heap, self, offsetof(struct string, actualString), stringObj);
  
  char* data = fluffygc_v1_get_object_critical(F->heap, stringObj, NULL);
  memcpy(data, string, len);
  data[len] = 0;
  fluffygc_v1_release_object_critical(F->heap, stringObj, data);
  fluffygc_v1_delete_local_ref(F->heap, stringObj);
  
  fluffygc_v1_obj_write_data(F->heap, self, offsetof(struct string, length), sizeof(len), &len);
  return (struct string_gcobject*) self;
  
failure:
  fluffygc_v1_delete_local_ref(F->heap, self);
  return NULL;
}

struct string_gcobject* string_from_cstring(struct vm* F, const char* string) {
  return string_new_string(F, string, strlen(string));
}

fluffygc_object* string_as_gcobject(struct string_gcobject* self) {
  return (fluffygc_object*) self;
}

const char* string_get_critical(struct vm* F, struct string_gcobject* self) {
  return fluffygc_v1_get_object_critical(F->heap, fluffygc_v1_get_object_field(F->heap, string_as_gcobject(self), offsetof(struct string, actualString)), NULL);
}

void string_release_critical(struct vm* F, struct string_gcobject* self, const char* criticalPtr) {
  return fluffygc_v1_release_object_critical(F->heap, fluffygc_v1_get_object_field(F->heap, string_as_gcobject(self), offsetof(struct string, actualString)), (void*) criticalPtr);
}






