#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#include "../../fluffyvm.h"
#include "../../format/bytecode.pb-c.h"
#include "../../parser/cjson.h"
#include "../../fluffyvm_types.h"
#include "json.h"

// cJSON not fully thread safe and
// i dont want read list of the part
// that not thread safe
static pthread_mutex_t cjson_lock = PTHREAD_MUTEX_INITIALIZER;

bool bytecode_loader_json_init(struct fluffyvm *vm) {
  return true;
}
void bytecode_loader_json_cleanup(struct fluffyvm *vm) {
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

    if (lowNum < 0 || lowNum > UINT32_MAX || floor(lowNum) != lowNum) {
      *errorMessage = "instruction.low is invalid"; 
      return false;
    } 
    
    if (highNum < 0 || highNum > UINT32_MAX || floor(highNum) != highNum) {
      *errorMessage = "instruction.high is invalid"; 
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
    for (int i = 0; i < bytecode->n_constants; i++)
      free(bytecode->constants[i]);
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

  for (int i = 0; i < prototype->n_instructions; i++) {
    cJSON* current = cJSON_GetArrayItem(instructions, i);
    double lowDouble;
    double highDouble;

    uint64_t low = (uint64_t) (lowDouble = cJSON_GetNumberValue(cJSON_GetObjectItem(current, "low")));
    uint64_t high = (uint64_t) (highDouble = cJSON_GetNumberValue(cJSON_GetObjectItem(current, "high")));
    
    prototype->instructions[i] = (high << 32) | low;
    //printf("A: Ins: %08X %08X\n",  (uint32_t) high, (uint32_t) low);
    //printf("   Ins: %lf %lf\n",  highDouble, lowDouble);
  }
  
  return prototype;
}

struct bytecode* bytecode_loader_json_load(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, const char* buffer, size_t len) {
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


  size_t packedLen = fluffy_vm_format__bytecode__bytecode__get_packed_size(&bytecode);
  void* data = malloc(packedLen);
  if (data == NULL) {
    freeBytecodeProtoBuf(&bytecode);
    value_copy(&vm->staticStrings.outOfMemory, &errorMessage2);
    goto error;
  }
  // printf("Serializing %zu bytes\n", packedLen);
  fluffy_vm_format__bytecode__bytecode__pack(&bytecode, data);
  
  //fwrite(data, 1, packedLen, stderr);
  //fflush(stderr);
  
  freeBytecodeProtoBuf(&bytecode);
  // String allocated by cJSON still being used until
  // this point.
  cJSON_Delete(json);
  pthread_mutex_unlock(&cjson_lock);
  
  // Error will be still properly propagates to
  // the caller
  struct bytecode* result = bytecode_load(vm, rootRef, data, packedLen);
  free(data);
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

  if (json) {
    cJSON_Delete(json);
    pthread_mutex_unlock(&cjson_lock);
  }

  if (*rootRef)
    foxgc_api_remove_from_root2(vm->heap, fluffyvm_get_root(vm), *rootRef);
  *rootRef = NULL;
  return NULL;
} 



