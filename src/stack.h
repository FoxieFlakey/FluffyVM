#ifndef header_1652093116_stack_h
#define header_1652093116_stack_h

// Non thread safe LIFO stack (Last In, First Out)
// Only accepts garbage collector
// managed objects

#include "foxgc.h"
#include "fluffyvm.h"

bool stack_init(struct fluffyvm* vm);
void stack_cleanup(struct fluffyvm* vm);

struct fluffyvm_stack {
  // Stack pointer
  int sp;
  int stackSize;
  foxgc_object_t** stack;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_stack;
};

struct fluffyvm_stack* stack_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, int stackSize);

// *result is the pointer to data contained in object
// false is underflow otherwise successful
bool stack_pop(struct fluffyvm* vm, struct fluffyvm_stack* stack, void** result, foxgc_root_reference_t** rootRef);

// false is overflow otherwise successful
bool stack_push(struct fluffyvm* vm, struct fluffyvm_stack* stack, foxgc_object_t* data);

#endif



