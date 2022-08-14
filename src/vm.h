#ifndef header_1660224236_3cbac18a_7b08_4f44_9966_495264b0f3fb_vm_h
#define header_1660224236_3cbac18a_7b08_4f44_9966_495264b0f3fb_vm_h

#include "FluffyGC/v1.h"

// The VM state
// I could use vm as the name but its too short
struct fluffyvm {
  fluffygc_state* heap;
};


struct fluffyvm* vm_new(fluffygc_state* heap);
void vm_free(struct fluffyvm* vm);

#endif

