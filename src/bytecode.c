#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bytecode.h"
#include "fluffyvm.h"
#include "fluffyvm_types.h"
#include "parser/cjson.h"
#include "value.h"
#include "format/bytecode.pb-c.h"

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

// cJSON not fully thread safe and
// i dont want read list of the part
// that not thread safe
static pthread_mutex_t cjson_lock = PTHREAD_MUTEX_INITIALIZER;

bool bytecode_init(struct fluffyvm* vm) {
  vm->bytecodeStaticData = malloc(sizeof(*vm->bytecodeStaticData));
  create_descriptor(desc_bytecode, struct fluffyvm_bytecode, {
    offsetof(struct fluffyvm_bytecode, gc_this),
    offsetof(struct fluffyvm_bytecode, gc_prototypes),
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

struct bytecode* bytecode_load(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, void* data, size_t len) {
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->bytecodeStaticData->desc_bytecode, NULL);
  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }
  struct bytecode* this = foxgc_api_object_get_data(obj);
  
  return this;
}

////////////////////////////////////////
// JSON loader                        //
////////////////////////////////////////

static bool validatePrototype(struct fluffyvm* vm, const char** errorMessage, struct value* errorMessage2, cJSON* prototype) {
  if (!cJSON_IsObject(prototype)) {
    *errorMessage = "prototype is not object";
    return false;
  }

  cJSON* prototypes = cJSON_GetObjectItem(prototype, "prototypes");
  cJSON* instructions = cJSON_GetObjectItem(prototype, "instructions");
  
  if (!cJSON_IsArray(prototypes)) { 
    *errorMessage = "prototype.prototypes is not array";
    return false;
  }
  
  if (!cJSON_IsArray(instructions)) { 
    *errorMessage = "prototype.instructions is not array";
    return false;
  }
   
  int arrayLen = cJSON_GetArraySize(instructions);
  for (int i = 0; i < arrayLen; i++) {
    cJSON* current = cJSON_GetArrayItem(instructions, i);
    if (!cJSON_IsObject(current)) {
      *errorMessage = "instruction is not object";
      return false;
    }

    cJSON* high = cJSON_GetObjectItem(current, "high");
    cJSON* low = cJSON_GetObjectItem(current, "low");
    
    if (!cJSON_IsNumber(high)) {
      *errorMessage = "instruction.high is not number";
      return false;
    }
    
    if (!cJSON_IsNumber(low)) {
      *errorMessage = "instruction.low is not number";
      return false;
    }

    double lowNum = cJSON_GetNumberValue(low);
    double highNum = cJSON_GetNumberValue(high);

    if (lowNum < 0 || lowNum > UINT32_MAX) {
      *errorMessage = "instruction.low exceeds 32-bit unsigned integer limit";
      return false;
    }
    
    if (highNum < 0 || highNum > UINT32_MAX) {
      *errorMessage = "instruction.high exceeds 32-bit unsigned integer limit";
      return false;
    }
    
    if (floor(lowNum) != lowNum) {
      *errorMessage = "instruction.low is not an integer";
      return false;
    }
    
    if (floor(highNum) != highNum) {
      *errorMessage = "instruction.high is not an integer";
      return false;
    }
  }

  arrayLen = cJSON_GetArraySize(prototypes);
  for (int i = 0; i < arrayLen; i++)
    if (!validatePrototype(vm, errorMessage, errorMessage2, cJSON_GetArrayItem(prototypes, i)))
      return false;

  return true;
}

static void freePrototypeProtoBuf(FluffyVmFormat__Bytecode__Prototype* prototype) {
  if (prototype->prototypes) {
    for (int i = 0; i < prototype->n_prototypes; i++) {
      FluffyVmFormat__Bytecode__Prototype* current = prototype->prototypes[i]; 
      if (current)
        freePrototypeProtoBuf(current);
    }
  }

  free(prototype->instructions);
  free(prototype->prototypes);
  free(prototype);
}

static void freeBytecodeProtoBuf(FluffyVmFormat__Bytecode__Bytecode* bytecode) {
  if (bytecode->constants) {
    for (int i = 0; i < bytecode->n_constants; i++) {
      FluffyVmFormat__Bytecode__Constant* current = bytecode->constants[i]; 
      if (current) {
        if (current->data_case == FLUFFY_VM_FORMAT__BYTECODE__CONSTANT__DATA_DATA_STR)
          free(current->data_str.data);
        free(current);
      }
    }
    free(bytecode->constants);
  }
  
  if (bytecode->mainprototype)
    freePrototypeProtoBuf(bytecode->mainprototype);
}

static FluffyVmFormat__Bytecode__Prototype* loadPrototype(struct fluffyvm* vm, const char** errorMessage, struct value* errorMessage2, cJSON* prototypeJson) {
  FluffyVmFormat__Bytecode__Prototype tmp = FLUFFY_VM_FORMAT__BYTECODE__PROTOTYPE__INIT;
  FluffyVmFormat__Bytecode__Prototype* prototype = malloc(sizeof(FluffyVmFormat__Bytecode__Prototype));
  if (!prototype) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }
  memcpy(prototype, &tmp, sizeof(*prototype));
  
  cJSON* prototypes = cJSON_GetObjectItem(prototypeJson, "prototypes");
  cJSON* instructions = cJSON_GetObjectItem(prototypeJson, "instructions");
 
  prototype->prototypes = NULL;
  prototype->instructions = NULL; 

  prototype->n_prototypes = cJSON_GetArraySize(prototypes);
  prototype->prototypes = calloc(prototype->n_prototypes, sizeof(FluffyVmFormat__Bytecode__Prototype*));
  if (prototype->prototypes == NULL) {
    freePrototypeProtoBuf(prototype);
    value_copy(&vm->staticStrings.outOfMemory, errorMessage2);
    return NULL;
  }
  
  for (int i = 0; i < prototype->n_prototypes; i++) {
    prototype->prototypes[i] = loadPrototype(vm, errorMessage, errorMessage2, cJSON_GetArrayItem(prototypes, i));
    if (prototype->prototypes[i] == NULL)
      return NULL;
  }

  prototype->n_instructions = cJSON_GetArraySize(instructions);
  prototype->instructions = calloc(prototype->n_instructions, sizeof(uint64_t));
  if (prototype->instructions == NULL) {
    freePrototypeProtoBuf(prototype);
    value_copy(&vm->staticStrings.outOfMemory, errorMessage2);
    return NULL;
  }

  return prototype;
}

struct bytecode* bytecode_load_json(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, const char* buffer, size_t len) {
  const char* errorMessage = NULL;
  struct value errorMessage2 = value_not_present();
  pthread_mutex_lock(&cjson_lock);
  *rootRef = NULL;

  // Validating JSON bytecode
  cJSON* json = cJSON_ParseWithLength(buffer, len);
  if (json == NULL) {
    errorMessage = "Error parsing";
    goto error;
  }

  if (!cJSON_IsObject(json)) {
    errorMessage = "JSON not object";
    goto error;
  }

  cJSON* constants = cJSON_GetObjectItem(json, "constants");
  cJSON* mainPrototype = cJSON_GetObjectItem(json, "mainPrototype");
  if (!cJSON_IsArray(constants)) { 
    errorMessage = "root.constants is not array";
    goto error;
  }
  
  // Validate constants
  int arrayLen = (int) cJSON_GetArraySize(constants);
  for (int i = 0; i < arrayLen; i++) {
    cJSON* current = cJSON_GetArrayItem(constants, i);
    if (!cJSON_IsObject(current)) {
      errorMessage = "invalid constant expect JSON object";
      goto error;
    }
    
    cJSON* type = cJSON_GetObjectItem(current, "type");
    cJSON* data = cJSON_GetObjectItem(current, "data");
   
    if (!cJSON_IsString(type)) {
      errorMessage = "invalid constant.type expect string";
      goto error;
    }

    if (strcmp("string", cJSON_GetStringValue(type)) == 0) {
      if (!cJSON_IsString(data)) {
        errorMessage = "invalid constant.data expect string";
        goto error;
      }
    } else {
      errorMessage = "unknown constant.type";
      goto error;
    }
  }
  
  // Validate main prototype
  if (!validatePrototype(vm, &errorMessage, &errorMessage2, mainPrototype))
    goto error;

  /*
  // Allocate resources
  foxgc_object_t* obj = foxgc_api_new_object(vm->heap, fluffyvm_get_root(vm), rootRef, vm->bytecodeStaticData->desc_bytecode, NULL);
  if (obj == NULL) {
    fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemory);
    return NULL;
  }
  struct bytecode* this = foxgc_api_object_get_data(obj);
  
  // Actually filling in the data
  */
  FluffyVmFormat__Bytecode__Bytecode bytecode = FLUFFY_VM_FORMAT__BYTECODE__BYTECODE__INIT;
  //////////////
  // Filling  //
  //////////////
  
  bytecode.constants = NULL;
  bytecode.mainprototype = NULL;
  bytecode.version = 1;
  
  bytecode.n_constants = cJSON_GetArraySize(constants);
  bytecode.constants = calloc(bytecode.n_constants, sizeof(FluffyVmFormat__Bytecode__Constant*));
  if (bytecode.constants == NULL) {
    freeBytecodeProtoBuf(&bytecode);
    value_copy(&vm->staticStrings.outOfMemory, &errorMessage2);
    goto error;
  }
 
  arrayLen = (int) cJSON_GetArraySize(constants);
  for (int i = 0; i < arrayLen; i++) {
    cJSON* current = cJSON_GetArrayItem(constants, i);  
    cJSON* type = cJSON_GetObjectItem(current, "type");
    cJSON* data = cJSON_GetObjectItem(current, "data");
    static FluffyVmFormat__Bytecode__Constant tmp = FLUFFY_VM_FORMAT__BYTECODE__CONSTANT__INIT;
    FluffyVmFormat__Bytecode__Constant* constant = malloc(sizeof(FluffyVmFormat__Bytecode__Constant));
    memcpy(constant, &tmp, sizeof(*constant));

    if (constant == NULL) {
      freeBytecodeProtoBuf(&bytecode);
      goto error;
    }
    
    bytecode.constants[i] = constant;
    constant->data_case = FLUFFY_VM_FORMAT__BYTECODE__CONSTANT__DATA__NOT_SET;
    constant->data_str.data = NULL;

    if (strcmp("string", cJSON_GetStringValue(type)) == 0) {
      constant->data_case = FLUFFY_VM_FORMAT__BYTECODE__CONSTANT__DATA_DATA_STR;
      constant->type = FLUFFY_VM_FORMAT__BYTECODE__CONSTANT_TYPE__STRING;
      constant->data_str.data = (uint8_t*) cJSON_GetStringValue(data);
      constant->data_str.len = strlen((char*) constant->data_str.data);
    }

    assert(constant->data_case != FLUFFY_VM_FORMAT__BYTECODE__CONSTANT__DATA__NOT_SET);
  }


  bytecode.mainprototype = loadPrototype(vm, &errorMessage, &errorMessage2, mainPrototype);
  if (bytecode.mainprototype == NULL) {
    freeBytecodeProtoBuf(&bytecode);
    goto error;
  }

  //////////////


  size_t packedLen = 0; fluffy_vm_format__bytecode__bytecode__get_packed_size(&bytecode);
  void* data = malloc(packedLen);
  if (data == NULL) {
    freeBytecodeProtoBuf(&bytecode);
    value_copy(&vm->staticStrings.outOfMemory, &errorMessage2);
    goto error;
  }
  //fluffy_vm_format__bytecode__bytecode__pack(&bytecode, data);
  
 
  
  struct bytecode* result = bytecode_load(vm, rootRef, data, packedLen);
  
  freeBytecodeProtoBuf(&bytecode);
  free(data);
 
  // String allocated by cJSON still being used until
  // this point. So freeing JSON data be deferred to
  // right before return
  cJSON_Delete(json);
  pthread_mutex_unlock(&cjson_lock);
  return result;

  error:
  if (errorMessage) {
    foxgc_root_reference_t* ref = NULL;
    struct value val = value_new_string(vm, errorMessage, &ref);
    if (val.type != FLUFFYVM_TVALUE_NOT_PRESENT) {
      fluffyvm_set_errmsg(vm, val);
      foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), ref);
    } else {
      fluffyvm_set_errmsg(vm, vm->staticStrings.outOfMemoryWhileAnErrorOccured);
    }
  } else if (errorMessage2.type != FLUFFYVM_TVALUE_NOT_PRESENT) {
    fluffyvm_set_errmsg(vm, errorMessage2);
  }
  cJSON_Delete(json);
  pthread_mutex_unlock(&cjson_lock);

  if (*rootRef)
    foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), *rootRef);
  *rootRef = NULL;
  return NULL;
} 







