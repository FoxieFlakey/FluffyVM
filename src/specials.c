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
  return "fast_unwind_on_malloc=0:"
         "detect_invalid_pointer_pairs=10:"
         "strict_string_checks=1:"
         "strict_init_order=1:"
         "check_initialization_order=1:"
         "print_stats=1:"
         "detect_stack_use_after_return=1:"
         "atexit=1";
}
#endif

#if IS_ENABLED(CONFIG_UBSAN)
const char* __ubsan_default_options() {
  return "print_stacktrace=1:"
         "suppressions=suppressions/UBSan.supp";
}
#endif

#if IS_ENABLED(CONFIG_TSAN)
const char* __tsan_default_options() {
  return "second_deadlock_stack=1";
}
#endif

#if IS_ENABLED(CONFIG_MSAN)
const char* __msan_default_options() {
  return "";
}
#endif

static const char* xrayOpts = "xray_mode=xray-basic:"
                              "patch_premain=true:"
                              "verbosity=1";

static void selfRestart(char** argv) {
  int err = setenv("XRAY_OPTIONS", xrayOpts, true);
  int errNum = errno;
  if (err < 0) {
    fputs("[Pre-main] Failed to set XRAY_OPTIONS: setenv: ", stderr);
    fputs(strerror(errNum), stderr);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
  }

  err = execvp(argv[0], argv);
  if (err < 0) {
    fputs("[Pre-main] Failed to self restart: execvp: ", stderr);
    fputs(strerror(errNum), stderr);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
  }
  exit(EXIT_FAILURE);
}

void special_premain(int argc, char** argv) {
  if (!IS_ENABLED(CONFIG_LLVM_XRAY) || true)
    return;

  const char* var;
  if ((var = getenv("XRAY_OPTIONS")))
    return;

  selfRestart(argv);
}


