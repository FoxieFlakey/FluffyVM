#ifndef header_1664455065_ca91d6b5_5c0f_4230_9aad_b5dd1301e644_util_h
#define header_1664455065_ca91d6b5_5c0f_4230_9aad_b5dd1301e644_util_h

#include <stdarg.h>
#include <stddef.h>

#include "compiler_config.h"

size_t util_vasprintf(char** buffer, const char* fmt, va_list args);

ATTRIBUTE_PRINTF(2, 3)
size_t util_asprintf(char** buffer, const char* fmt, ...);

typedef void (^runnable_block)();

// Tiny useful macros from Linux kernel
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define FIELD_SIZEOF(t, f) (sizeof(((t*) NULL)->f))

#define __stringify_1(x...)	#x
#define stringify(x...)	__stringify_1(x)

#ifdef __GNUC__
# define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
#else
# define container_of(ptr, type, member) ((type *)( (char *)__mptr - offsetof(type,member) )) 
#endif

#endif



