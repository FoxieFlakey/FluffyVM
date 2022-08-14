#ifndef header_1656164800_compiler_config_h
#define header_1656164800_compiler_config_h

////////////////////////////////////////
// Compiler config                    //
////////////////////////////////////////

#ifdef __GNUC__
# define ATTRIBUTE(x) __attribute__(x)
#else
# define ATTRIBUTE(x)
#endif

#ifdef __has_feature
# if __has_feature(address_sanitizer)
#  define CONFIG_ASAN 1
# endif
# if __has_feature(thread_sanitizer)
#  define CONFIG_TSAN 1
# endif
# if __has_feature(undefined_behavior_sanitizer)
#  define CONFIG_UBSAN 1
# endif
# if __has_feature(memory_sanitizer)
#  define CONFIG_MSAN 1
# endif
#endif

#endif

