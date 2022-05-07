#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bytecode.h"
#include "fluffyvm.h"
#include "fluffyvm_types.h"
#include "value.h"
#include "format/bytecode.pb-c.h"
#include "config.h"

#define create_descriptor(name, structure, ...) do { \
  size_t offsets[] = __VA_ARGS__; \
  vm->bytecodeStaticData->name = foxgc_api_descriptor_new(vm->heap, sizeof(offsets) / sizeof(offsets[0]), offsets, sizeof(structure)); \
  if (vm->bytecodeStaticData->name == NULL) \
    return false; \
} while (0)

#define free_descriptor(name) do { \
  if (vm->bytecodeStaticData->name) \
    foxgc_api_descriptor_remove(vm->bytecodeStaticData->name); \
} while(0)

bool bytecode_init(struct fluffyvm* vm) {
  vm->bytecodeStaticData = malloc(sizeof(*vm->bytecodeStaticData));
  create_descriptor(desc_bytecode, struct fluffyvm_bytecode, {
    offsetof(struct fluffyvm_bytecode, gc_this),
    offsetof(struct fluffyvm_bytecode, gc_constants),
    offsetof(struct fluffyvm_bytecode, gc_mainPrototype),
    offsetof(struct fluffyvm_bytecode, gc_constantsObject),
  });
  
  create_descriptor(desc_prototype, struct fluffyvm_bytecode, {
    offsetof(struct fluffyvm_prototype, gc_this),
    offsetof(struct fluffyvm_prototype, gc_bytecode),
    offsetof(struct fluffyvm_prototype, gc_instructions),
  });

  return true;
}

void bytecode_cleanup(struct fluffyvm* vm) {
  free_descriptor(desc_bytecode);
  free_descriptor(desc_prototype);
  free(vm->bytecodeStaticData);
}

static inline void bytecode_write_constants_array(struct fluffyvm_bytecode* bytecode, foxgc_object_t* obj) {
  foxgc_api_write_field(bytecode->gc_this, 1, obj);
  if (obj) {
    bytecode->constants = foxgc_api_object_get_data(obj);
    bytecode->constants_len = foxgc_api_get_array_length(obj);
  }
}

static inline void bytecode_write_main_prototype(struct fluffyvm_bytecode* bytecode, struct fluffyvm_prototype* obj) {
  if (obj) {
    foxgc_api_write_field(bytecode->gc_this, 2, obj->gc_this);
    bytecode->mainPrototype = foxgc_api_object_get_data(obj->gc_this);
  }
}

static inline void bytecode_write_constants_object_array(struct fluffyvm_bytecode* bytecode, foxgc_object_t* obj) {
  foxgc_api_write_field(bytecode->gc_this, 3, obj);
  if (obj)
    bytecode->constantsObject = foxgc_api_object_get_data(obj);
}

static inline void bytecode_write_constant(struct fluffyvm* vm, struct fluffyvm_bytecode* this, int index, struct value constant) {
  foxgc_object_t* ptr;
  if ((ptr = value_get_object_ptr(constant)))
    foxgc_api_write_array(this->gc_constantsObject, index, ptr);
  value_copy(&this->constants[index], &constant);
  //fwrite(value_get_string(this->constants[index]), 1, value_get_len(this->constants[index]), stdout);
}

static struct fluffyvm_prototype* loadPrototype(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, FluffyVmFormat__Bytecode__Prototype* proto) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->bytecodeStaticData->desc_prototype, NULL);
  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }

  struct fluffyvm_prototype* this = foxgc_api_object_get_data(obj);
  // Write to this->gc_this
  foxgc_api_write_field(obj, 0, obj);

  // Noop for a while because
  // right now im testing the
  // constant pool loading and
  // different constant types

  return this;
} 

struct fluffyvm_bytecode* bytecode_load(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, void* data, size_t len) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->bytecodeStaticData->desc_bytecode, NULL);
  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }
  struct fluffyvm_bytecode* this = foxgc_api_object_get_data(obj);
  // Write to this->gc_this
  foxgc_api_write_field(obj, 0, obj);
 
  // Decode
  FluffyVmFormat__Bytecode__Bytecode* bytecode = fluffy_vm_format__bytecode__bytecode__unpack(NULL, len, data);
  if (bytecode == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.protobufFailedToUnpackData);
    goto error;
  }

  if (bytecode->version != FLUFFYVM_RELEASE_NUM) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.unsupportedBytecode);
    goto error;
  }
  
  foxgc_root_reference_t* constantsRef = NULL;
  foxgc_object_t* constantsArray = foxgc_api_new_data_array(vm->heap, fluffyvm_get_root(vm), &constantsRef, sizeof(struct value), bytecode->n_constants, NULL);
  if (!constantsArray)
    goto no_memory;
  bytecode_write_constants_array(this, constantsArray);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), constantsRef);
  
  foxgc_root_reference_t* constantsObjectArrayRef = NULL;
  foxgc_object_t* constantsObjectArray = foxgc_api_new_array(vm->heap, fluffyvm_get_root(vm), &constantsObjectArrayRef, bytecode->n_constants, NULL);
  if (!constantsObjectArray)
    goto no_memory;
  bytecode_write_constants_object_array(this, constantsObjectArray);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), constantsObjectArrayRef);
  
  for (int i = 0; i < bytecode->n_constants; i++) {
    FluffyVmFormat__Bytecode__Constant* current = bytecode->constants[i];
    struct value val = value_not_present();
    foxgc_root_reference_t* tmpRootRef = NULL;

    switch (current->type) {
      case FLUFFY_VM_FORMAT__BYTECODE__CONSTANT_TYPE__STRING:
        if (current->data_case != FLUFFY_VM_FORMAT__BYTECODE__CONSTANT__DATA_DATA_STR) {
          fluffyvm_set_errmsg(vm, vm->staticStrings.invalidBytecode);
          goto error;
        }
        struct value tmp = value_new_string2(vm, (char*) current->data_str.data, current->data_str.len, &tmpRootRef);
        value_copy(&val, &tmp);
        break;
      case FLUFFY_VM_FORMAT__BYTECODE__CONSTANT_TYPE__LONG:
        if (current->data_case != FLUFFY_VM_FORMAT__BYTECODE__CONSTANT__DATA_DATA_LONG_NUM) {
          fluffyvm_set_errmsg(vm, vm->staticStrings.invalidBytecode);
          goto error;
        } 
        struct value tmp2 = value_new_long(vm, current->data_longnum);
        value_copy(&val, &tmp2);
        break;
      case FLUFFY_VM_FORMAT__BYTECODE__CONSTANT_TYPE__DOUBLE:
        if (current->data_case != FLUFFY_VM_FORMAT__BYTECODE__CONSTANT__DATA_DATA_DOUBLE_NUM) {
          fluffyvm_set_errmsg(vm, vm->staticStrings.invalidBytecode);
          goto error;
        }
        struct value tmp3 = value_new_double(vm, current->data_doublenum);
        value_copy(&val, &tmp3);
        break;
      // Loader does not recognize other type
      default:
        fluffyvm_set_errmsg(vm, vm->staticStrings.invalidBytecode);
        goto error;
    }

    if (val.type == FLUFFYVM_TVALUE_NOT_PRESENT) {
      fluffyvm_set_errmsg(vm, vm->staticStrings.invalidBytecode);
      goto error;
    }

    if (tmpRootRef)
      foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmpRootRef);

    bytecode_write_constant(vm, this, i, val);
  }

  foxgc_root_reference_t* mainPrototypeRef = NULL;
  struct fluffyvm_prototype* mainPrototype = loadPrototype(vm, &mainPrototypeRef, bytecode->mainprototype);
  if (!mainPrototype)
    goto error;
  bytecode_write_main_prototype(this, mainPrototype);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), mainPrototypeRef);

  fluffy_vm_format__bytecode__bytecode__free_unpacked(bytecode, NULL);
  return this;

  no_memory:
  fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
  
  error:
  if (bytecode)
    fluffy_vm_format__bytecode__bytecode__free_unpacked(bytecode, NULL);

  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), *rootRef);
  *rootRef = NULL;
  return NULL;
}








