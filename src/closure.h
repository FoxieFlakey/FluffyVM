#ifndef header_1652075735_closure_h
#define header_1652075735_closure_h

#include "bytecode.h"
#include "coroutine.h"
#include "fluffyvm.h"

struct fluffyvm_closure {
  struct fluffyvm_prototype* prototype;
  
  // _ENV table
  struct value env;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_prototype;
  foxgc_object_t* gc_env;
};

bool closure_init(struct fluffyvm* vm);
void closure_cleanup(struct fluffyvm* vm);

struct fluffyvm_closure* closure_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, struct fluffyvm_prototype* prototype, struct value env);

#endif

