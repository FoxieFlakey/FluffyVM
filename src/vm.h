#ifndef header_1660224236_3cbac18a_7b08_4f44_9966_495264b0f3fb_vm_h
#define header_1660224236_3cbac18a_7b08_4f44_9966_495264b0f3fb_vm_h

#include <pthread.h>
#include <stdatomic.h>
#include <FluffyGC/v1.h>

struct vm {
  fluffygc_state* heap;

  atomic_int threadAttachedCount;

  pthread_key_t osThreadLocalDataKey;
  
  bool osThreadLocalDataKeyInited;
};

struct vm* vm_new(fluffygc_state* heap);
void vm_free(struct vm* self);

#endif

