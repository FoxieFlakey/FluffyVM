#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "list.h"
#include "queue.h"

struct queue* queue_new(int size) {
  struct queue* this = malloc(sizeof(*this));
  pthread_mutex_init(&this->lock, NULL);
  pthread_cond_init(&this->readySignal, NULL);
  pthread_cond_init(&this->taskArriveSignal, NULL);
  
  this->queue = list_new();
  this->itemsInQueue = 0;
  this->queueSize = size;
  return this;
}

void queue_destroy(struct queue* this) {
  list_destroy(this->queue);
  pthread_cond_destroy(&this->readySignal);
  pthread_cond_destroy(&this->taskArriveSignal);
  pthread_mutex_destroy(&this->lock);
  free(this);
}

void queue_enqueue(struct queue* this, void* data) {
  pthread_mutex_lock(&this->lock);
  if (this->itemsInQueue >= this->queueSize) 
    pthread_cond_wait(&this->readySignal, &this->lock);
  
  list_rpush(this->queue, list_node_new(data));
  this->itemsInQueue++;
  
  pthread_mutex_unlock(&this->lock);
  pthread_cond_signal(&this->taskArriveSignal);
}

bool queue_enqueue_nonblocking(struct queue* this, void* data) {
  pthread_mutex_lock(&this->lock);
  if (this->itemsInQueue >= this->queueSize) {
    pthread_mutex_unlock(&this->lock);
    return false;
  }
  
  list_rpush(this->queue, list_node_new(data));
  this->itemsInQueue++;
  
  pthread_mutex_unlock(&this->lock);
  pthread_cond_signal(&this->taskArriveSignal);
  return true;
}

void queue_remove(struct queue* this, void** data) {
  pthread_mutex_lock(&this->lock);
  
  if (this->itemsInQueue <= 0) 
    pthread_cond_wait(&this->taskArriveSignal, &this->lock);
  
  list_node_t* node = list_at(this->queue, 0);
  this->itemsInQueue--;
  *data = node->val;
  list_remove(this->queue, node); 
  
  pthread_mutex_unlock(&this->lock);
  pthread_cond_signal(&this->readySignal);
}

bool queue_remove_nonblocking(struct queue* this, void** data) {
  pthread_mutex_lock(&this->lock);
  
  if (this->itemsInQueue <= 0) {
    pthread_mutex_unlock(&this->lock);
    return false;
  }
  
  list_node_t* node = list_at(this->queue, 0);
  this->itemsInQueue--;
  *data = node->val;
  list_remove(this->queue, node); 
  
  pthread_mutex_unlock(&this->lock);
  pthread_cond_signal(&this->readySignal);
  
  return true;
}


