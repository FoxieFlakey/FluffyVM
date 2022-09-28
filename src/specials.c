#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

#if IS_ENABLED(CONFIG_ASAN)
const char* __asan_default_options() {
  return CONFIG_ASAN_OPTS;
}
#endif

#if IS_ENABLED(CONFIG_UBSAN)
const char* __ubsan_default_options() {
  return CONFIG_UBSAN_OPTS;
}
#endif

#if IS_ENABLED(CONFIG_TSAN)
const char* __tsan_default_options() {
  return CONFIG_TSAN_OPTS;
}
#endif

#if IS_ENABLED(CONFIG_MSAN)
const char* __msan_default_options() {
  return CONFIG_MSAN_OPTS;
}
#endif

#if IS_ENABLED(CONFIG_LLVM_XRAY)
static const char* xrayOpts = CONFIG_LLVM_XRAY_OPTS;
#else
static const char* xrayOpts = "";
#endif

static void selfRestart(char** argv) {
  int err = setenv("XRAY_OPTIONS", xrayOpts, true);
  int errNum = errno;
  if (err < 0) {
    fputs("[Boot] Failed to set XRAY_OPTIONS: setenv: ", stderr);
    fputs(strerror(errNum), stderr);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
  }

  err = execvp(argv[0], argv);
  if (err < 0) {
    fputs("[Boot] Failed to self restart: execvp: ", stderr);
    fputs(strerror(errNum), stderr);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
  }

  fputs("[Boot] Can't reached here!!!", stderr);
  exit(EXIT_FAILURE);
}

void special_premain(int argc, char** argv) {
  if (!IS_ENABLED(CONFIG_LLVM_XRAY))
    return;

  const char* var;
  if ((var = getenv("XRAY_OPTIONS"))) {
    fprintf(stderr, "[Boot] Success XRAY_OPTIONS now contain \"%s\"\n", var);
    return;
  }

  fputs("[Boot] Self restarting due XRAY_OPTIONS not set\n", stderr);
  selfRestart(argv);
}


