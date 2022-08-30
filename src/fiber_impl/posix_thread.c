#include <stdatomic.h>
#include <stdlib.h>

#include "fiber.h"
#include "fiber_impl/posix_thread.h"

static void* threadEntryPoint(void* _self) {
  struct fiber_posix* self = _self;
  fiber_impl_posix_wait_command(self);

  self->entryPoint(self->owner);
  fiber_impl_posix_send_command_completion(self, NULL);
  return NULL;
}

struct fiber_posix* fiber_impl_posix_posix_new(void (*entryPoint)(struct fiber*), struct fiber* owner, struct fiber_posix** alsoSaveTo) {
  struct fiber_posix* self = malloc(sizeof(*self));
  if (!self)
    return NULL;

  if (alsoSaveTo)
    *alsoSaveTo = self;
  
  self->owner = owner;
  self->entryPoint = entryPoint;
  self->commandCompleted = false;
  self->commandReceived = false;
  self->firstCommand = true; 

  self->barrierInited = false;
  self->threadInited = false;
  self->commandReceivedLockInited = false;
  self->commandReceivedCondInited = false;
  self->commandCompletedCondInited = false;
  self->commandCompletedLockInited = false;
    
  if (pthread_cond_init(&self->commandCompletedCond, NULL) != 0)
    goto failure;
  self->commandCompletedCondInited = true;

  if (pthread_mutex_init(&self->commandCompletedLock, NULL) != 0)
    goto failure;
  self->commandCompletedLockInited = true;
  
  if (pthread_cond_init(&self->commandReceivedCond, NULL) != 0)
    goto failure;
  self->commandReceivedCondInited = true;
   
  if (pthread_mutex_init(&self->commandReceivedLock, NULL) !=0)
    goto failure;
  self->commandReceivedLockInited = true;

  if (pthread_barrier_init(&self->barrier, NULL, 2) != 0)
    goto failure;
  self->barrierInited = true;

  if (pthread_create(&self->thread, NULL, threadEntryPoint, self) != 0) 
    goto failure;
  self->threadInited = true;

  pthread_barrier_wait(&self->barrier);
  return self;

  failure:
  fiber_impl_posix_free(self);
  return NULL;
}

void fiber_impl_posix_free(struct fiber_posix* self) {
  if (self->threadInited && atomic_load(&self->owner->state) != FIBER_DEAD)
    fiber_impl_posix_send_command_and_wait(self, FIBER_COMMAND_TERMINATE, NULL); 
  if (self->barrierInited)
    pthread_barrier_destroy(&self->barrier);
  if (self->commandCompletedCondInited)
    pthread_cond_destroy(&self->commandCompletedCond);
  if (self->commandCompletedLockInited)
    pthread_mutex_destroy(&self->commandCompletedLock);
  if (self->commandReceivedCondInited)
    pthread_cond_destroy(&self->commandReceivedCond);
  if (self->commandReceivedLockInited)
    pthread_mutex_destroy(&self->commandReceivedLock);
  free(self);
}

void fiber_impl_posix_send_command_and_wait(struct fiber_posix* self, enum fiber_command command, void** result) {
  if (!self->firstCommand)
    pthread_barrier_wait(&self->barrier);
  self->firstCommand = false;

  pthread_mutex_lock(&self->commandCompletedLock); 
  pthread_mutex_lock(&self->commandReceivedLock);
  self->command = command;
  self->commandReceived = true;
  pthread_cond_signal(&self->commandReceivedCond);
  pthread_mutex_unlock(&self->commandReceivedLock);
  
  while (!self->commandCompleted)
    pthread_cond_wait(&self->commandCompletedCond, &self->commandCompletedLock);

  self->commandCompleted = false;
  pthread_mutex_unlock(&self->commandCompletedLock);
}

void fiber_impl_posix_send_command_completion(struct fiber_posix* self, void* result) {
  pthread_mutex_lock(&self->commandCompletedLock);
  self->commandCompleted = true;
  pthread_cond_signal(&self->commandCompletedCond);
  pthread_mutex_unlock(&self->commandCompletedLock);
}

enum fiber_command fiber_impl_posix_wait_command(struct fiber_posix* self) {
  pthread_mutex_lock(&self->commandReceivedLock);
  pthread_barrier_wait(&self->barrier);

  while (!self->commandReceived)
    pthread_cond_wait(&self->commandReceivedCond, &self->commandReceivedLock);
  
  enum fiber_command cmd = self->command;
  self->commandReceived = false;
  pthread_mutex_unlock(&self->commandReceivedLock);
  return cmd;
}



