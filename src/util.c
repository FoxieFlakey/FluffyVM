#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "util.h"

size_t util_vasprintf(char** buffer, const char* fmt, va_list args) {
  va_list copy;
  va_copy(copy, args);
  size_t size = vsnprintf(NULL, 0, fmt, copy);
  va_end(copy);

  *buffer = malloc(size + 1);
  if (*buffer == NULL)
    return -ENOMEM;

  return vsnprintf(*buffer, size + 1, fmt, args);
}

size_t util_asprintf(char** buffer, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t ret = util_vasprintf(buffer, fmt, args);
  va_end(args);
  return ret;
}

