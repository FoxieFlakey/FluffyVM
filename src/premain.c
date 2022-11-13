#include <stdio.h>
#include <pthread.h>

#include "specials.h"
#include "config.h"
#include "bug.h"

// Pre-main
int main2(int argc, char** argv);

struct worker_args {
  int* ret;
  int argc;
  char** argv;
};

static void* testWorker(void* _args) {
  struct worker_args* args = _args;
  *args->ret = main2(args->argc, args->argv);
  return NULL;
}

int main(int argc, char** argv) {
  special_premain(argc, argv);

  int res = 0;
  struct worker_args args = {
    .ret = &res,
    .argc = argc,
    .argv = argv
  };
  
  if (IS_ENABLED(CONFIG_DONT_START_SEPERATE_MAIN_THREAD)) {
    testWorker(&args);
  } else {
    pthread_t tmp;
    int res = pthread_create(&tmp, NULL, testWorker, &args);
    BUG_ON(res != 0);
    pthread_join(tmp, NULL);
  }

  puts("Exiting :3");
  return res;
}



