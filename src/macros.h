#ifndef header_1660723678_5668e9a7_9fa8_4e10_aaf0_497547423daa_macros_h
#define header_1660723678_5668e9a7_9fa8_4e10_aaf0_497547423daa_macros_h

#include <stddef.h>

#define container_of(ptr, type, member) \
  ((type*) ((ptr) - ((typeof(((type*) 0)->member)*) offsetof(type, member))))
#define array_size(x) (sizeof(x) / sizeof(x[0]))
#define field_sizeof(type, field) sizeof(((type*) 0)->field)

#endif 

