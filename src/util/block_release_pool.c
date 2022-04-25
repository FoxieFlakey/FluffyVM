#include <stdlib.h>
#include <Block.h>

#include "block_release_pool.h"

const void* block_pool_add(block_release_pool_t* pool, const void* block) {
  if (pool->blocks == NULL) {
    pool->blocks = list_new();
  }
  list_rpush(pool->blocks, list_node_new((void*) block));
  
  return block;
}

void block_pool_release(block_release_pool_t* pool) {
  if (pool->blocks == NULL)
    return;
  
  list_iterator_t* it = list_iterator_new(pool->blocks, LIST_HEAD);
  list_node_t* node;
  while ((node = list_iterator_next(it))) {
    Block_release(node->val);
  }
  list_iterator_destroy(it);
  
  list_destroy(pool->blocks);
  pool->blocks = NULL;
}

void block_pool_release2(block_release_pool_t** pool) {
  block_pool_release(*pool); 
}
