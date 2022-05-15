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
  if (!vm->bytecodeStaticData)
    return false;

  create_descriptor(desc_bytecode, struct fluffyvm_bytecode, {
    offsetof(struct fluffyvm_bytecode, gc_this),
    offsetof(struct fluffyvm_bytecode, gc_constants),
    offsetof(struct fluffyvm_bytecode, gc_mainPrototype),
    offsetof(struct fluffyvm_bytecode, gc_constantsObject),
  });
  
  create_descriptor(desc_prototype, struct fluffyvm_prototype, {
    offsetof(struct fluffyvm_prototype, gc_this),
    offsetof(struct fluffyvm_prototype, gc_bytecode),
    offsetof(struct fluffyvm_prototype, gc_instructions),
    offsetof(struct fluffyvm_prototype, gc_prototypes),
  });

  return true;
}

void bytecode_cleanup(struct fluffyvm* vm) {
  if (!vm->bytecodeStaticData)
    return;

  free_descriptor(desc_bytecode);
  free_descriptor(desc_prototype);
  free(vm->bytecodeStaticData);
}

// Prototype setters //

static inline void prototype_write_prototypes_array(struct fluffyvm_prototype* proto, foxgc_object_t* obj) {
  foxgc_api_write_field(proto->gc_this, 3, obj);
  if (obj) {
    proto->prototypes = foxgc_api_object_get_data(obj);
    proto->prototypes_len = foxgc_api_get_array_length(obj);
  } else {
    proto->prototypes = NULL;
    proto->prototypes_len = 0;
  }
}

static inline void prototype_write_prototype(struct fluffyvm_prototype* proto, int index, struct fluffyvm_prototype* proto2) {
  foxgc_api_write_array(proto->gc_prototypes, index, proto2->gc_this);
}

static inline void prototype_write_instructions_array(struct fluffyvm_prototype* proto, foxgc_object_t* obj) {
  foxgc_api_write_field(proto->gc_this, 2, obj);
  if (obj) {
    proto->instructions = foxgc_api_object_get_data(obj);
    proto->instructions_len = foxgc_api_get_array_length(obj);
  } else {
    proto->instructions = NULL;
    proto->instructions_len = 0;
  }
}

static inline void prototype_write_bytecode(struct fluffyvm_prototype* proto, struct fluffyvm_bytecode* bytecode) {
  proto->bytecode = bytecode;
  foxgc_api_write_field(proto->gc_this, 1, bytecode ? bytecode->gc_this : NULL);
}

///////////////////////

static struct fluffyvm_prototype* loadPrototype(struct fluffyvm* vm, struct fluffyvm_bytecode* bytecode, foxgc_root_reference_t** rootRef, FluffyVmFormat__Bytecode__Prototype* proto) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->bytecodeStaticData->desc_prototype, NULL);
  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }

  // Allocate resources  
  struct fluffyvm_prototype* this = foxgc_api_object_get_data(obj);
  // Write to this->gc_this
  foxgc_api_write_field(obj, 0, obj);
  prototype_write_bytecode(this, bytecode);

  foxgc_root_reference_t* prototypesRef = NULL;
  foxgc_object_t* prototypesArray = foxgc_api_new_array(vm->heap, fluffyvm_get_root(vm), &prototypesRef, proto->n_prototypes, NULL);
  if (!prototypesArray)
    goto no_memory;
  prototype_write_prototypes_array(this, prototypesArray);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), prototypesRef);
  
  foxgc_root_reference_t* instructionsRef = NULL;
  foxgc_object_t* instructionsArray = foxgc_api_new_data_array(vm->heap, fluffyvm_get_root(vm), &instructionsRef, sizeof(fluffyvm_instruction_t), proto->n_instructions, NULL);
  if (!instructionsArray)
    goto no_memory;
  prototype_write_instructions_array(this, instructionsArray);
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), instructionsRef);
  
  // Filling data
  for (int i = 0; i < proto->n_prototypes; i++) {
    foxgc_root_reference_t* tmpRootRef = NULL;
    struct fluffyvm_prototype* tmp = loadPrototype(vm, bytecode, &tmpRootRef, proto->prototypes[i]);
    if (!tmp)
      goto error;
    prototype_write_prototype(this, i, tmp);
    foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmpRootRef);
  }

  for (int i = 0; i < proto->n_instructions; i++)
    this->instructions[i] = (fluffyvm_instruction_t) proto->instructions[i];

  return this;
  
  no_memory:
  fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
  
  error:
  foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), *rootRef);
  *rootRef = NULL;
  return NULL;
} 

// Bytecode setters //

static inline void bytecode_write_constants_array(struct fluffyvm_bytecode* bytecode, foxgc_object_t* obj) {
  foxgc_api_write_field(bytecode->gc_this, 1, obj);
  if (obj) {
    bytecode->constants = foxgc_api_object_get_data(obj);
    bytecode->constants_len = foxgc_api_get_array_length(obj);
  } else {
    bytecode->constants = NULL;
    bytecode->constants_len = 0;
  }
}

static inline void bytecode_write_main_prototype(struct fluffyvm_bytecode* bytecode, struct fluffyvm_prototype* obj) {
  foxgc_api_write_field(bytecode->gc_this, 2, obj ? obj->gc_this : NULL);
  bytecode->mainPrototype = obj;
}

static inline void bytecode_write_constants_object_array(struct fluffyvm_bytecode* bytecode, foxgc_object_t* obj) {
  foxgc_api_write_field(bytecode->gc_this, 3, obj);
  if (obj)
    bytecode->constantsObject = foxgc_api_object_get_data(obj);
  else
    bytecode->constantsObject = NULL;
}

static void bytecode_write_constant(struct fluffyvm* vm, struct fluffyvm_bytecode* this, int index, struct value constant) {
  foxgc_object_t* ptr;
  if ((ptr = value_get_object_ptr(constant)))
    foxgc_api_write_array(this->gc_constantsObject, index, ptr);
  else
    foxgc_api_write_array(this->gc_constantsObject, index, NULL);
  value_copy(&this->constants[index], &constant);
}

//////////////////////

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
  
  // Allocate resources
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
  
  // Filling data
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

    bytecode_write_constant(vm, this, i, val);
    if (tmpRootRef)
      foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), tmpRootRef);
  }

  foxgc_root_reference_t* mainPrototypeRef = NULL;
  struct fluffyvm_prototype* mainPrototype = loadPrototype(vm, this, &mainPrototypeRef, bytecode->mainprototype);
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

// Getters
struct value bytecode_get_constant(struct fluffyvm* vm, struct fluffyvm_bytecode* this, foxgc_root_reference_t** rootRef, int index) {
  if (index < 0 || index >= this->constants_len) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.invalidArrayBound);
    return value_not_present();
  }
  
  foxgc_object_t* ptr = value_get_object_ptr(this->constants[index]);
  if (ptr)
    foxgc_api_root_add(vm->heap, ptr, fluffyvm_get_root(vm), rootRef);

  return this->constants[index];
}

struct fluffyvm_prototype* bytecode_prototype_get_prototype(struct fluffyvm* vm, struct fluffyvm_prototype* this, foxgc_root_reference_t** rootRef, int index) {
  if (index < 0 || index >= this->prototypes_len) { 
    fluffyvm_set_errmsg(vm, vm->staticStrings.invalidArrayBound);
    return NULL;
  }

  foxgc_api_root_add(vm->heap, this->prototypes[index], fluffyvm_get_root(vm), rootRef);
  return foxgc_api_object_get_data(this->prototypes[index]);
}





