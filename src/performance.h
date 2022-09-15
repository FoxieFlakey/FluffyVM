#ifndef header_1662804198_f919a476_780e_459c_86dc_cff16de154bb_performance_h
#define header_1662804198_f919a476_780e_459c_86dc_cff16de154bb_performance_h

#include "config.h"
#include "attributes.h"

#if IS_ENABLED(CONFIG_ADDITIONAL_INLINING)
# define PERF_POSSIBLE_HOT_FORCE_INLINE ATTRIBUTE_ALWAYS_INLINE
#else
# define PERF_POSSIBLE_HOT_FORCE_INLINE
#endif

#endif

