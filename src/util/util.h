#ifndef _headers_1639487495_MinecraftLauncherCLI_v2_util
#define _headers_1639487495_MinecraftLauncherCLI_v2_util

#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "functional/functional.h"
#include "../config.h"

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

int util_vasprintf(char** strp, const char* fmt, va_list ap);

ATTRIBUTE((format(printf, 2, 3))) 
int util_asprintf(char** strp, const char* fmt, ...);

ATTRIBUTE((format(printf, 2, 3))) 
const char* util_string_format(int* size, const char* fmt, ...);

//void util_print_backtrace_from_buffer(PrintWriter* writer, void** buffer, int size);
//void util_print_backtrace_from_current(PrintWriter* writer, int backtraceSize);

// Crossplatform timegm
time_t util_timegm(struct tm* t);

// Returns seconds since 1970
// Returns -1 for fail to parse (as there no such -1 seconds since epoch lol)
time_t util_parse_iso8601(const char* input);

// True if matched
bool util_is_matched(const char* str, const char* regex);

char* util_realpath(const char* path);

int util_get_online_core_count();

// Length is the length of output buffer
bool util_hex2bin(const char* str, uint8_t* buffer, size_t length);

// Need to be free after use
char* util_bin2hex(uint8_t* src, size_t len);

// Start new thread
bool util_run_thread(runnable_t runnable, pthread_t* thread);

// This interface should be flexible enough
// This is inplace rotation Positive for right 
// rotate and negative for left rotate
typedef void (^util_array_set)(int index, void* data);
typedef void* (^util_array_get)(int index);
void util_collections_rotate(int len, int distance, util_array_set setter, util_array_get getter);

#define UTIL_TO_STRING_impl(x) #x
#define UTIL_TO_STRING(x) UTIL_TO_STRING_impl(x)
#define UTIL_SOURCE_LOCATION (__FILE_NAME__ ":" UTIL_TO_STRING(__LINE__))

#endif


