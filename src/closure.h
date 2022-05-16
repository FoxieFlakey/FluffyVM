#ifndef header_1652075735_closure_h
#define header_1652075735_closure_h

#include "bytecode.h"
#include "fluffyvm.h"

struct fluffyvm_call_state;

// Note: Any closure can yield across
//       C function boundry if you dont
//       call `coroutine_disallow_yield`
typedef bool (*closure_cfunction_t)(struct fluffyvm* vm, struct fluffyvm_call_state* callState, void* udata);

// Warning: The finalizer for the udate
//          must be minimum and cannot
//          access the VM state because
//          finalizer can be triggered
//          long after the VM state is
//          discarded
typedef void (*closure_udata_finalizer_t)(void* udata);

struct fluffyvm_closure {
  struct fluffyvm_prototype* prototype;
  closure_cfunction_t func; 
  void* udata;
  closure_udata_finalizer_t finalizer;

  // _ENV table
  struct value env;

  foxgc_object_t* gc_this;
  foxgc_object_t* gc_prototype;
  foxgc_object_t* gc_env;
};

bool closure_init(struct fluffyvm* vm);
void closure_cleanup(struct fluffyvm* vm);

struct fluffyvm_closure* closure_new(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, struct fluffyvm_prototype* prototype, struct value env);
struct fluffyvm_closure* closure_from_cfunction(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, closure_cfunction_t func, void* udata, closure_udata_finalizer_t finalizer, struct value env);

#endif

