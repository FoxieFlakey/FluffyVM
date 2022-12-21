#include <FluffyGC/v1.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>

#include "bug.h"
#include "vm_array_primitive.h"
#include "vm.h"
#include "value.h"
#include "util.h"
#include "constants.h"

static const char* subystemKey = "I'm a";

#define get_private(F) (F->array_primitiveData)

int array_primitive_init(struct vm* F) {
  F->array_primitiveData = malloc(sizeof(*F->array_primitiveData));
  if (!F->array_primitiveData)
    return -ENOMEM;
  
  fluffygc_field fields[] = {
    {
      .name = "valueArray",
      .offset = offsetof(struct primitive_array, valueArray),
      .dataType = FLUFFYGC_TYPE_NORMAL,
      .type = FLUFFYGC_FIELD_STRONG
    },
    {
      .name = "refArray",
      .offset = offsetof(struct primitive_array, refArray),
      .dataType = FLUFFYGC_TYPE_ARRAY,
      .type = FLUFFYGC_FIELD_STRONG
    }
  };
  fluffygc_descriptor_args tmp = {
    .name = VM_PACKAGE ".PrimitiveArray",
    .objectSize = sizeof(struct primitive_array),
    .fieldCount = ARRAY_SIZE(fields),
    .fields = fields,
    .typeID = (uintptr_t) &subystemKey 
  };
  
  get_private(F)->desc = fluffygc_v1_descriptor_new(F->heap, &tmp);
  if (!get_private(F)->desc)
    return -ENOMEM;
  
  return 0;
}

void array_primitive_cleanup(struct vm* F) {
  if (!get_private(F))
    return;
  
  fluffygc_v1_descriptor_delete(F->heap, get_private(F)->desc);
  free(get_private(F));
}

struct array_primitive_gcobject* array_primitive_new(struct vm* F, size_t len) {
  if (len > INT_MAX)
    return NULL;
  
  if (fluffygc_v1_push_frame(F->heap, 3) < 0)
    return NULL;
  
  fluffygc_object* self = fluffygc_v1_new_object(F->heap, get_private(F)->desc);
  if (!self)
    return NULL;
  
  fluffygc_object* valueArrayObj = fluffygc_v1_new_opaque_object(F->heap, sizeof(struct value) * len);
  if (!valueArrayObj)
    goto failure;
  fluffygc_v1_set_object_field(F->heap, self, offsetof(struct primitive_array, valueArray), valueArrayObj);
  fluffygc_v1_obj_write_data(F->heap, self, offsetof(struct primitive_array, len), FIELD_SIZEOF(struct primitive_array, len), &len);
  
  fluffygc_object_array* refArrayObj = fluffygc_v1_new_object_array(F->heap, len);
  if (!refArrayObj)
    goto failure;
  fluffygc_v1_set_array_field(F->heap, self, offsetof(struct primitive_array, refArray), refArrayObj);
  
  return (struct array_primitive_gcobject*) fluffygc_v1_pop_frame(F->heap, self);
  
failure:
  fluffygc_v1_pop_frame(F->heap, NULL);
  return NULL;
}

size_t array_primitive_get_length(struct vm* F, struct array_primitive_gcobject* self) {
  size_t len;
  fluffygc_v1_obj_read_data(F->heap, cast_to_gcobj(self), offsetof(struct primitive_array, len), sizeof(len), &len);
  return len;
}

static int commonWrite(struct vm* F, struct array_primitive_gcobject* self, int index, struct value data, fluffygc_object* obj) {
  if (fluffygc_v1_push_frame(F->heap, 2) < 0)
    return -ENOMEM;
  
  size_t len;
  fluffygc_v1_obj_read_data(F->heap, cast_to_gcobj(self), offsetof(struct primitive_array, len), sizeof(len), &len);
  if (index < 0 || index >= len)
    return -ERANGE;
  
  fluffygc_object* array = fluffygc_v1_get_object_field(F->heap, cast_to_gcobj(self), offsetof(struct primitive_array, valueArray));
  fluffygc_object_array* refArray = fluffygc_v1_get_array_field(F->heap, cast_to_gcobj(self), offsetof(struct primitive_array, refArray));
  
  value_set_gcobject(F, &data, NULL);
  fluffygc_v1_set_object_array_element(F->heap, refArray, index, obj);
  fluffygc_v1_obj_write_data(F->heap, array, sizeof(data) * index, sizeof(data), &data);
  
  fluffygc_v1_pop_frame(F->heap, NULL);
  return 0;
}

static int commonRead(struct vm* F, struct array_primitive_gcobject* self, int index, struct value* data, fluffygc_object** obj) {
  if (fluffygc_v1_push_frame(F->heap, 3) < 0)
    return -ENOMEM;
  
  size_t len;
  fluffygc_v1_obj_read_data(F->heap, cast_to_gcobj(self), offsetof(struct primitive_array, len), sizeof(len), &len);
  if (index < 0 || index >= len)
    return -ERANGE;
  
  fluffygc_object* array = fluffygc_v1_get_object_field(F->heap, cast_to_gcobj(self), offsetof(struct primitive_array, valueArray));
  fluffygc_object_array* refArray = fluffygc_v1_get_array_field(F->heap, cast_to_gcobj(self), offsetof(struct primitive_array, refArray));
  
  fluffygc_v1_obj_read_data(F->heap, array, sizeof(*data) * index, sizeof(*data), &data);
  fluffygc_object* byrefObj = fluffygc_v1_get_object_array_element(F->heap, refArray, index);
  
  if (obj)
    *obj = fluffygc_v1_pop_frame(F->heap, byrefObj);
  else
    fluffygc_v1_pop_frame(F->heap, NULL);
  return 0;
}

int array_primitive_set_byref(struct vm* F, struct array_primitive_gcobject* self, int index, struct value data) {
  if (!value_is_byref(F, data))
    return -EINVAL;
  return commonWrite(F, self, index, data, value_get_gcobject(F, data));
}

int array_primitive_set_byval(struct vm* F, struct array_primitive_gcobject* self, int index, struct value data) {
  if (!value_is_byval(F, data))
    return -EINVAL;
  return commonWrite(F, self, index, data, NULL);
}

int array_primitive_set(struct vm* F, struct array_primitive_gcobject* self, int index, struct value data) {
  
  return 0;
}

int array_primitive_get_byref(struct vm* F, struct array_primitive_gcobject* self, int index, struct value* result) {
  struct value val;
  fluffygc_object* obj;
}
