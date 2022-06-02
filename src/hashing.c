#include <xxhash.h>
#include <stdlib.h>

#include "hashing.h"
#include "config.h"

uint64_t hashing_hash_default(const void* data, size_t size) {
  if (FLUFFYVM_HASH_USE_FNV)
    return hashing_hash_fnv(data, size);
  else if (FLUFFYVM_HASH_USE_OPENJDK8)
    return hashing_hash_openjdk8(data, size);
  else if (FLUFFYVM_HASH_USE_XXHASH)
    return hashing_hash_xxhash(data, size);
  
  abort();
}

uint64_t hashing_hash_xxhash(const  void* data, size_t size) {
  return (uint64_t) XXH64(data, size, FLUFFYVM_XXHASH_SEED);
}

uint64_t hashing_hash_fnv(const void* data, size_t size) {
  uint64_t hash = FLUFFYVM_FNV_OFFSET_BASIS;

  for (int i = 0; i < size; i++) {
    hash ^= ((const uint8_t*) data)[i];
    hash *= FLUFFYVM_FNV_PRIME;
  }
  return hash;
}

uint64_t hashing_hash_openjdk8(const void* data, size_t size) {
  uint64_t hash = 0;

  for (int i = 0; i < size; i++)
    hash = 31 * hash + ((uint8_t*) data)[i];

  return hash;
}


