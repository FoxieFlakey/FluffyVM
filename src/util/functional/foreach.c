#include <Block.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "foreach.h"
#include "../util.h"
#include "functional.h"

foreach_status_t foreach_parallel_do(supplier_t supplier, consumer_t consumer, int threads) {
  if (threads == 0)
    threads = util_get_online_core_count();
  __block atomic_int foreachStatus = FOREACH_OK;
  __block atomic_bool mustStop = false;
  __block pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
  pthread_t* threadsList = malloc(sizeof(pthread_t) * threads);
  
  supplier_t safeSupplier = ^void* () {
    pthread_mutex_lock(&mtx);
    void* tmp = supplier();
    pthread_mutex_unlock(&mtx);
    return tmp;
  };
  
  // Starting threads
  for (int i = 0; i < threads; i++) {
    util_run_thread(^void () {
      void* tmp;
      while ((tmp = safeSupplier())) {
        if (consumer(tmp) == false) {
          foreachStatus = FOREACH_FAILED;
          break;
        }
        
        if (mustStop)
          break;
      }
      
      mustStop = true; 
    }, &threadsList[i]);
  }
  
  // Waiting for threads
  for (int i = 0; i < threads; i++) {
    pthread_join(threadsList[i], NULL);
  }
  
  free(threadsList);
  pthread_mutex_destroy(&mtx);
  return foreachStatus;
}

foreach_status_t foreach_do(supplier_t supplier, consumer_t consumer) {
  void* tmp;
  while ((tmp = supplier())) {
    if (consumer(tmp) == false) {
      return FOREACH_FAILED;
    }
  }
  
  return FOREACH_OK;
}

foreach_status_t foreach_do_list_direction(list_t* list, list_direction_t dir, consumer_t consumer) {
  list_iterator_t* it = list_iterator_new(list, dir);
  supplier_t supplier = functional_from_list_iterator(it);
  foreach_status_t status = foreach_do(supplier, consumer);
  
  Block_release(supplier);
  list_iterator_destroy(it);
  return status;
}

foreach_status_t foreach_do_list(list_t* list, consumer_t consumer) {
  list_iterator_t* it = list_iterator_new(list, LIST_HEAD);
  supplier_t supplier = functional_from_list_iterator(it);
  foreach_status_t status = foreach_do(supplier, consumer);
  
  Block_release(supplier);
  list_iterator_destroy(it);
  return status;
}

