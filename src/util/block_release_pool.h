#ifndef _headers_1639647917_MinecraftLauncherCLI_v2_release_pool
#define _headers_1639647917_MinecraftLauncherCLI_v2_release_pool

#include <stddef.h>
#include "../collections/list.h"

typedef struct {
  list_t* blocks;
} block_release_pool_t;

const void* block_pool_add(block_release_pool_t* pool, const void* block);

//Release all the blocks associated and make the pool reusable
void block_pool_release(block_release_pool_t* pool);

#define BLOCK_POOL_AUTO_RELEASE __attribute__((cleanup(block_pool_release)))

#endif

