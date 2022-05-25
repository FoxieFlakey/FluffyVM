#include "fluffyvm.h"
#include "fluffyvm_types.h"
#include "stack.h"

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#define create_descriptor(name, structure, ...) do { \
  size_t offsets[] = __VA_ARGS__; \
  vm->stackStaticData->name = foxgc_api_descriptor_new(vm->heap, sizeof(offsets) / sizeof(offsets[0]), offsets, sizeof(structure)); \
  if (vm->stackStaticData->name == NULL) \
    return false; \
} while (0)

#define free_descriptor(name) do { \
  if (vm->stackStaticData->name) \
    foxgc_api_descriptor_remove(vm->stackStaticData->name); \
} while(0)

#define STACK_OFFSET_THIS (0)
#define STACK_OFFSET_STACK (1)

bool stack_init(struct fluffyvm* vm) {
  if (vm->stackStaticData)
    return true;

  vm->stackStaticData = malloc(sizeof(*vm->stackStaticData));
  if (!vm->stackStaticData)
    return false;
  
  create_descriptor(desc_stack, struct fluffyvm_stack, {
    offsetof(struct fluffyvm_stack, gc_this),
    offsetof(struct fluffyvm_stack, gc_stack)
  });

  return true;
}

void stack_cleanup(struct fluffyvm* vm) {
  if (vm->stackStaticData) {
    free_descriptor(desc_stack);
    free(vm->stackStaticData);
  }
}

struct fluffyvm_stack* stack_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, int stackSize) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->stackStaticData->desc_stack, NULL);
  if (!obj) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }

  struct fluffyvm_stack* this = foxgc_api_object_get_data(obj);
  foxgc_api_write_field(obj, 0, obj);
  this->stackSize = stackSize;
  this->sp = 0;

  foxgc_root_reference_t* tmp = NULL;
  foxgc_object_t* stackObj = foxgc_api_new_array(vm->heap, fluffyvm_get_root(vm), &tmp, stackSize, NULL);
  if (!stackObj)
    goto no_memory;
  this->stack = foxgc_api_object_get_data(stackObj);
  foxgc_api_write_field(obj, STACK_OFFSET_STACK, stackObj);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmp);
  return this;

  no_memory:
  fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), *rootRef);
  *rootRef = NULL;
  return NULL;
}

bool stack_pop(struct fluffyvm* vm, struct fluffyvm_stack* stack, void** result, foxgc_root_reference_t** rootRef) {
  if (stack->sp - 1 < 0) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.stackUnderflow);
    return false;
  } 

  stack->sp--;
  if (result) {
    foxgc_object_t* obj = stack->stack[stack->sp];
    foxgc_api_root_add(vm->heap, obj, fluffyvm_get_root(vm), rootRef);
    *result = foxgc_api_object_get_data(obj);
  }

  foxgc_api_write_array(stack->gc_stack, stack->sp, NULL);
  return true; 
}

bool stack_push(struct fluffyvm* vm, struct fluffyvm_stack* stack, foxgc_object_t* data) {
  if (stack->sp >= stack->stackSize) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.stackOverflow);
    return false;
  }

  foxgc_api_write_array(stack->gc_stack, stack->sp, data);
  stack->sp++;
  return true;
}

bool stack_peek(struct fluffyvm* vm, struct fluffyvm_stack* stack, void** result) {
  if (stack->sp - 1 < 0)
    return false;

  *result = foxgc_api_object_get_data(stack->stack[stack->sp - 1]);
  return true;
}

