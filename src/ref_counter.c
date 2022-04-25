#include <stdatomic.h>
#include <stdlib.h>
#include <assert.h>

#include "ref_counter.h"

struct ref_counter* ref_counter_new(void* data, ref_counter_finalizer finalizer) {
  struct ref_counter* this = malloc(sizeof(*this));
  this->counter = 1;
  this->data = data;
  this->finalizer = finalizer;
  return this;
}

void ref_counter_inc(struct ref_counter* ref) {
  int prev = atomic_fetch_add(&ref->counter, 1);
  assert(prev > 0);
}

void ref_counter_dec(struct ref_counter* ref) {
  int prev = atomic_fetch_sub(&ref->counter, 1);
  assert(prev > 0);
  if (prev == 1) {
    ref->finalizer(ref);
    free(ref);
  }
}







