#ifndef header_1660463755_52fbaa0e_d15f_43af_9fac_230e91a6a51d_vm_types_h
#define header_1660463755_52fbaa0e_d15f_43af_9fac_230e91a6a51d_vm_types_h

#include <stdint.h>
#include <inttypes.h>

typedef int64_t vm_int;
typedef uint64_t vm_uint;
typedef double vm_number;

typedef uint64_t vm_instruction;

#define PRINT_VM_INT_X PRIX64
#define PRINT_VM_INT_x PRIx64
#define PRINT_VM_INT_d PRId64
#define PRINT_VM_INT_i PRIi64
#define PRINT_VM_INT_u PRIu64

#define PRINT_VM_NUMBER "lf"

#endif

