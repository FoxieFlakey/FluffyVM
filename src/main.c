#include <stdlib.h>
#include <stdio.h>
#include <Block.h>

#include <FluffyGC/v1.h>
#include <sys/ucontext.h>
#include <ucontext.h>

#include "coroutine.h"
#include "fiber.h"
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
  
  ucontext_t ctx;
  ucontext_t ctx2;
  ucontext_t tmp;

  getcontext(&ctx2);
  
  puts("fox does squeks");
  
  tmp = ctx;
  ctx = ctx2;
  ctx2 = tmp;
  swapcontext(&ctx2, &ctx);
  
  puts("fox does squeks");
  
  tmp = ctx;
  ctx = ctx2;
  ctx2 = tmp;
  swapcontext(&ctx2, &ctx);
  
  struct vm* F = vm_new(heap);
  __block struct coroutine* co = coroutine_new(F, (struct coroutine_exec_state) {
    .canRelease = false,
    .block = ^void () {
      puts("Hello");
      fiber_yield();
      puts("World");
      fiber_yield();
      puts("!");
    } 
  });

  printf("%d\n", fiber_resume(co->fiber, NULL));
  printf("%d\n", fiber_resume(co->fiber, NULL));
  //fiber_resume(co->fiber, NULL);
  //fiber_resume(co->fiber, NULL);

  coroutine_free(co);
  vm_free(F);
  fluffygc_v1_free(heap);
  return EXIT_SUCCESS;
}

