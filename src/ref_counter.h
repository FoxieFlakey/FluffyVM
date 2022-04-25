#ifndef header_1650778015_ref_counter_h
#define header_1650778015_ref_counter_h

#include <stdatomic.h>

struct ref_counter;

typedef void (^ref_counter_finalizer)(struct ref_counter*);

typedef struct ref_counter {
  atomic_int counter;
  ref_counter_finalizer finalizer;
  
  void* data;
} ref_counter_t;

struct ref_counter* ref_counter_new(void* data, ref_counter_finalizer finalizer);
void ref_counter_inc(struct ref_counter* counter);
void ref_counter_dec(struct ref_counter* counter);

#endif

