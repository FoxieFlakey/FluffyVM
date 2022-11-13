#ifndef _headers_1667308371_FluffyVM_bytecode_deserializer_protobuf
#define _headers_1667308371_FluffyVM_bytecode_deserializer_protobuf

#include <stddef.h>

struct bytecode;

// `result` and `size` assumed to be non NULL as it doesnt make sense 
// if these were NULL and `version` is set to version even error are thrown 
// (except for not enough memory where it set to 0)
// Return zero on success
// Errors:
// -ENOMEM: Not enough memory
// -EFAULT: Failure deserializing
// -EINVAL: Invalid bytecode (except if version is zero it mean unrecognized version)
int bytecode_deserializer_protobuf(struct bytecode** result, int* version, void* input, size_t size);

#endif

