#ifndef _headers_1667611741_FluffyVM_vm_array
#define _headers_1667611741_FluffyVM_vm_array

// Primitive array
// its storing value which is value based and doesnt contain GC references
#include <FluffyGC/v1.h>

struct vm;
struct value;

struct array_primitive_subsystem_data {
  fluffygc_descriptor* desc;
};

struct array {
  size_t size;
  fluffygc_object* actualArray; 
};

int array_primitive_init(struct vm* F);
void array_primitive_cleanup(struct vm* F);

struct array_primitive_gcobject {};

struct array_primitive_gcobject* array_primitive_new(struct vm* F, size_t len);

size_t array_primitive_get_length(struct vm* F, struct array_primitive_gcobject* self);
int array_primitive_set(struct vm* F, struct array_primitive_gcobject* self, int index, struct value obj);

// Return result type
int array_primitive_get(struct vm* F, struct array_primitive_gcobject* self, int index, struct value* result);

#endif

