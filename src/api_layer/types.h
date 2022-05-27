#ifndef header_1653537788_types_h
#define header_1653537788_types_h

#include <stdint.h>

// Numbers
typedef int64_t fluffyvm_integer;
typedef uint64_t fluffyvm_unsigned;
typedef double fluffyvm_number;

// Structs 
typedef struct fluffyvm fluffyvm;
typedef struct fluffyvm_call_state fluffyvm_call_state;

// Functions
typedef int (*fluffyvm_cfunction)(fluffyvm* F, fluffyvm_call_state* callState, void* udata);

#endif

