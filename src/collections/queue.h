#ifndef _headers_1643029318_FGGC_v2_queue
#define _headers_1643029318_FGGC_v2_queue

#include <bits/types/struct_timespec.h>
#include <pthread.h>
#include <stdbool.h>

#include "list.h"

typedef struct queue {
  pthread_mutex_t lock;
  pthread_cond_t readySignal;
  pthread_cond_t taskArriveSignal;
  
  // Size of the queue
  int queueSize;
  int itemsInQueue;
  
  // void*
  list_t* queue;
} queue_t;

struct queue* queue_new(int size); 
void queue_destroy(struct queue* queue);

void queue_enqueue(struct queue* queue, void* data);
void queue_remove(struct queue* queue, void** data);

bool queue_enqueue_nonblocking(struct queue* queue, void* data);
bool queue_remove_nonblocking(struct queue* this, void** data);

bool queue_timed_enqueue(struct queue* queue, void* data, struct timespec* timeout);
bool queue_timed_remove(struct queue* queue, void** data, struct timespec* timeout);

#endif

