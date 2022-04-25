#include "util.h"

#include <stdio.h> /* needed for vsnprintf */
#include <stdlib.h> /* needed for malloc-free */
#include <stdarg.h> /* needed for va_list */
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <regex.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <Block.h>

#include "functional/functional.h"

// Reindented from 
// https://stackoverflow.com/a/58037981/13447666
static int days_from_epoch(int y, int m, int d) {
  y -= m <= 2;
  int era = y / 400;
  int yoe = y - era * 400;                                   // [0, 399]
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;  // [0, 365]
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;           // [0, 146096]
  return era * 146097 + doe - 719468;
}

// Reindented from 
// https://stackoverflow.com/a/58037981/13447666
time_t util_timegm(struct tm* t) {
  int year = t->tm_year + 1900;
  int month = t->tm_mon;          // 0-11
  if (month > 11) {
    year += month / 12;
    month %= 12;
  } else if (month < 0) {
    int years_diff = (11 - month) / 12;
    year -= years_diff;
    month += 12 * years_diff;
  }
  int days_since_epoch = days_from_epoch(year, month + 1, t->tm_mday);
  
  return 60 * (60 * (24L * days_since_epoch + t->tm_hour) + t->tm_min) + t->tm_sec;
}

// Modified from https://stackoverflow.com/a/46480387/13447666
static int _vscprintf_so(const char * format, va_list pargs) {
  int retval;
  va_list argcopy;
  va_copy(argcopy, pargs);
  retval = vsnprintf(NULL, 0, format, argcopy);
  va_end(argcopy);
  return retval;
}

// Modified from https://stackoverflow.com/a/46480387/13447666
int util_vasprintf(char **strp, const char *fmt, va_list ap) {
  int len = _vscprintf_so(fmt, ap);
  if (len == -1) return -1;
  char *str = malloc((size_t) len + 1);
  if (!str) return -1;
  int r = vsnprintf(str, len + 1, fmt, ap); /* "secure" version of vsprintf */
  if (r == -1) return free(str), -1;
  *strp = str;
  return r;
}

// Modified from https://stackoverflow.com/a/46480387/13447666
int util_asprintf(char **strp, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = util_vasprintf(strp, fmt, ap);
  va_end(ap);
  return r;
} 

const char* util_string_format(int* size, const char* fmt, ...) {
  char* buffer;
  va_list ap;
  va_start(ap, fmt);
  int r = util_vasprintf(&buffer, fmt, ap);
  if (size != NULL) {
    *size = r;
  }
  va_end(ap);
  return buffer;
}

static int ParseInt(const char* value, bool* success) {
  char* endptr = NULL;
  long result = strtol(value, &endptr, 10);
  *success = !(endptr == value);
  return result;
}

// Returns seconds since 1970
// Returns -1 for fail to parse (as there no such -1 seconds since epoch lol)
time_t util_parse_iso8601(const char* input) {
  const size_t expectedLength = sizeof("1234-12-12T12:12:12Z") - 1;
  
  if (strlen(input) < expectedLength) {
    return -1; 
  }
  
  struct tm time = {0};
  bool status = false;
  time.tm_year = ParseInt(&input[0], &status) - 1900; if (!status) return -1;
  time.tm_mon = ParseInt(&input[5], &status) - 1;     if (!status) return -1;
  time.tm_mday = ParseInt(&input[8], &status);        if (!status) return -1;
  time.tm_hour = ParseInt(&input[11], &status);       if (!status) return -1;
  time.tm_min = ParseInt(&input[14], &status);        if (!status) return -1;
  time.tm_sec = ParseInt(&input[17], &status);        if (!status) return -1;
  time.tm_isdst = 0;
  
  time_t res = util_timegm(&time);
  if (res == -1)
    return -1;
  return res;
}

bool util_is_matched(const char* str, const char* regex) {
  regex_t regex_struct;
  if (regcomp(&regex_struct, regex, REG_EXTENDED | REG_NOSUB) != 0)
    return false;
  
  bool result = regexec(&regex_struct, str, 1, NULL, 0) == 0;
  
  regfree(&regex_struct);
  return result;
}

char* util_realpath(const char* path) {
  return strdup(path);
}

int util_get_online_core_count() {
  return sysconf(_SC_NPROCESSORS_ONLN);
}

// https://stackoverflow.com/a/68509642/13447666
bool util_hex2bin(const char* str, uint8_t* buffer, size_t length) {
  char buf[3];
  size_t i;
  int value;
  for (i = 0; i < length && *str; i++) {
    buf[0] = *str++;
    buf[1] = '\0';
    if (*str) {
      buf[1] = *str++;
      buf[2] = '\0';
    }
    if (sscanf(buf, "%x", &value) != 1)
      break;
    buffer[i] = value;
  }
  
  return i == length * 2;
}

char* util_bin2hex(uint8_t* src, size_t len) {
  char* str = NULL;
  for (int i = 0; i < len; i++) {
    printf("%02X", src[i]);
  }
  printf("\n");
  
  return str;
}

static void* runner(void* arg) {
  ((runnable_t) arg)();
  return NULL;
}

bool util_run_thread(runnable_t runnable, pthread_t* thread) {
  pthread_t t;
  if (runnable == NULL)
    return false;
  
  if (thread == NULL)
    thread = &t;
  
  return pthread_create(thread, NULL, runner, runnable) == 0;
}
