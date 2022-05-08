#ifndef header_1651987028_thread_h
#define header_1651987028_thread_h

#include "bytecode.h"
#include "fluffyvm.h"
#include "value.h"
#include "foxgc.h"

// Currently executing function
struct fluffyvm_call_state {
  struct fluffyvm_prototype* prototype;
  uint32_t pc;

  struct value registers[1 << 16];
  foxgc_object_t** registersObjectArray;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_registerObjectArray;
  foxgc_object_t* gc_prototype;
};

typedef enum {
  FLUFFYVM_COROUTINE_STATE_RUNNING,
  FLUFFYVM_COROUTINE_STATE_DEAD,
  FLUFFYVM_COROUTINE_STATE_SUSPENDED
} fluffyvm_coroutine_state;

struct fluffyvm_coroutine {
  // If this running on a
  // native thread the coroutine
  // cannot be yielded
  bool isNativeThread;
  fluffyvm_coroutine_state state;
 
  struct fluffyvm_call_state* currentCallState;
  struct fluffyvm_bytecode* bytecode;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_bytecode;
};

bool coroutine_init(struct fluffyvm* vm);
void coroutine_cleanup(struct fluffyvm* vm);

#endif



