#include <stdlib.h>
#include <FluffyGC/v1.h>

#include "vm.h"

#define KiB * 1024
#define MiB * 1024 KiB
#define GiB * 1024 MiB
#define TiB * 1024 GiB

int main2() {
  fluffygc_state* heap = fluffygc_v1_new(
      8 MiB, 
      16 MiB, 
      64 KiB, 
      100, 
      0.45f,
      65536);
  struct vm* vm = vm_new(heap);



  vm_free(vm);
  fluffygc_v1_free(heap);
  return EXIT_SUCCESS;
}

