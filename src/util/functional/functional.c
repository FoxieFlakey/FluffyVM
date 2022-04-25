#include "functional.h"
#include <Block.h>

supplier_t functional_from_list_iterator(list_iterator_t* iterator) {
  return Block_copy(^void* () {
    list_node_t* node = list_iterator_next(iterator);
    if (!node) {
      return NULL;
    }
    
    return node->val;
  });
}


