#include <errno.h>
#include <stdlib.h>

#include "protobuf_deserializer.h"
#include "bytecode.h"
#include "prototype.h"
#include "format/bytecode.pb-c.h"
#include "util.h"
#include "constants.h"
#include "vec.h"
#include "vm_types.h"

// Sanity checks (to catch mistakes of changing sizes without changing the proto file)
static_assert(sizeof(vm_instruction) == FIELD_SIZEOF(FluffyVmFormat__Bytecode__Prototype, instructions[0]));
static_assert(sizeof(vm_int) == FIELD_SIZEOF(FluffyVmFormat__Bytecode__Constant, data_integer));
static_assert(sizeof(vm_number) == FIELD_SIZEOF(FluffyVmFormat__Bytecode__Constant, data_number));

static int processConstant(struct bytecode* bytecode, FluffyVmFormat__Bytecode__Constant* constantProtoBuf) {
  int res = 0;
  
  switch (constantProtoBuf->data_case) {
    case FLUFFY_VM_FORMAT__BYTECODE__CONSTANT__DATA_DATA_INTEGER:
      res = bytecode_add_constant_int(bytecode, constantProtoBuf->data_integer);
      break;
    case FLUFFY_VM_FORMAT__BYTECODE__CONSTANT__DATA_DATA_NUMBER:
      res = bytecode_add_constant_number(bytecode, constantProtoBuf->data_number);
      break;
    case FLUFFY_VM_FORMAT__BYTECODE__CONSTANT__DATA_DATA_STR:
      res = bytecode_add_constant_string(bytecode, (char*) constantProtoBuf->data_str.data, constantProtoBuf->data_str.len);
      break;
    default:
      // TODO: Decide whether to log this to VM's printk buffer or fail
      res = -EINVAL;
      break;
  }
  
  return res;
}

static int convertPrototype(struct bytecode* owner, struct prototype** result, FluffyVmFormat__Bytecode__Prototype* prototypeProtoBuf) {
  int res = 0;
  struct prototype* proto = prototype_new();
  if (!proto)
    return -ENOMEM;
  proto->owner = owner;
  
  if ((res = prototype_set_code(proto, prototypeProtoBuf->n_instructions, prototypeProtoBuf->instructions)) < 0) {
    if (res == -E2BIG)
      res = -EINVAL;
    goto save_instructions_error;
  }
  
save_instructions_error:
  if (res < 0) {
    prototype_free(proto);
    proto = NULL;
  }
  *result = proto;
  return res;
}

static int convertBytecode(struct bytecode** result, FluffyVmFormat__Bytecode__Bytecode* bytecodeProtoBuf) {
  int res = 0;
  struct bytecode* bytecode = bytecode_new();
  if (!bytecode)
    return -ENOMEM;

  if (vec_reserve(&bytecode->constants, bytecodeProtoBuf->n_constants) < 0) {
    res = -ENOMEM;
    goto reserve_constants_error;
  }
  
  for (int i = 0; i < bytecodeProtoBuf->n_constants; i++) 
    if ((res = processConstant(bytecode, bytecodeProtoBuf->constants[i])) < 0)
      goto constant_convert_error;

  if ((res = convertPrototype(bytecode, &bytecode->mainPrototype, bytecodeProtoBuf->mainprototype)) < 0)
    goto prototype_convert_error;
  
  *result = bytecode; 
prototype_convert_error:
constant_convert_error:
reserve_constants_error:
  if (res < 0) 
    bytecode_free(bytecode);
  
  return res;
}

int bytecode_deserializer_protobuf(struct bytecode** result, int* versionResult, void* input, size_t size) {
  int res = 0;
  int version = 0;
  FluffyVmFormat__Bytecode__Bytecode* bytecode = fluffy_vm_format__bytecode__bytecode__unpack(NULL, size, input);
  if (bytecode == NULL) {
    res = -ENOMEM;
    goto not_enough_memory;
  }
  version = bytecode->version;
  
  if (bytecode->version != VM_BYTECODE_VERSION) {
    res = -EINVAL;
    goto invalid_version;
  }
  
  if ((res = convertBytecode(result, bytecode)) < 0) 
    goto fail_converting;

fail_converting:
invalid_version:
  fluffy_vm_format__bytecode__bytecode__free_unpacked(bytecode, NULL);
not_enough_memory:
  if (versionResult)
    *versionResult = version;
  return res;
}
