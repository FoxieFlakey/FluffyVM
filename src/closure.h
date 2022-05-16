#ifndef header_1652075735_closure_h
#define header_1652075735_closure_h

#include "bytecode.h"
#include "fluffyvm.h"

struct fluffyvm_call_state;
typedef void (^closure_block_function_t)(struct fluffyvm_call_state* callState);

struct fluffyvm_closure {
  struct fluffyvm_prototype* prototype;
  closure_block_function_t func; 

  // _ENV table
  struct value env;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_prototype;
  foxgc_object_t* gc_env;
};

bool closure_init(struct fluffyvm* vm);
void closure_cleanup(struct fluffyvm* vm);

struct fluffyvm_closure* closure_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, struct fluffyvm_prototype* prototype, struct value env);
struct fluffyvm_closure* closure_from_block(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, closure_block_function_t func, struct value env);

#endif

