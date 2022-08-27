#include <stdlib.h>
#include <stdio.h>
#include <Block.h>

#include <FluffyGC/v1.h>

#include "coroutine.h"
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
  struct vm* F = vm_new(heap);
  struct coroutine* co = coroutine_new(F, (struct coroutine_exec_state) {
    .canRelease = false,
    .block = ^void (struct coroutine* co, void* arg) {
      puts(arg);

      co->exec = (struct coroutine_exec_state) {
        .canRelease = true,
        .block = Block_copy(^void (struct coroutine* co, void* arg) {
          puts(arg);
        })
      };
      //co->state = COROUTINE_PAUSED;
      return coroutine_yield(co);
    } 
  });

  coroutine_resume(co, "Hello From Resume");
  coroutine_resume(co, "Hello From Resume2");
  coroutine_free(co);
  
  vm_free(F);
  fluffygc_v1_free(heap);
  return EXIT_SUCCESS;
}

