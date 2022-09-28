#include <stdio.h>
#include <pthread.h>

#include "specials.h"
#include "config.h"

// Pre-main
int main2();

static void* testWorker(void* res) {
  *((int*) res) = main2();
  return NULL;
}

int main(int argc, char** argv) {
  special_premain(argc, argv);

  int res = 0;
  if (IS_ENABLED(CONFIG_DONT_START_SEPERATE_MAIN_THREAD)) {
    testWorker(&res);
  } else {
    pthread_t tmp;
    pthread_create(&tmp, NULL, testWorker, &res);
    pthread_join(tmp, NULL);
  }

  puts("Exiting :3");
  return res;
}



