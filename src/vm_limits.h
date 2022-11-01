#ifndef _headers_1667304735_FluffyVM_vm_limits
#define _headers_1667304735_FluffyVM_vm_limits

#include <stdint.h>

// This limit is per prototype minus 1
#define VM_LIMIT_MAX_CODE (UINT32_C(1) << 31)

// This limit is per bytecode minus 1
#define VM_LIMIT_MAX_CONSTANT (UINT32_C(1) << 31)

// This limit is per prototype minus 1
// Currently limited by rxi/vec limitation of not supporting 4 billions
// entries due its `length` field is an int and 2 billions already
// larger than any practical purpose
#define VM_LIMIT_MAX_PROTOTYPE (UINT32_C(1) << 30)

#define VM_REGISTER_COUNT 16

#endif

