#ifndef header_1654068994_hashing_h
#define header_1654068994_hashing_h

#include <stdint.h>
#include <stddef.h>

// Hash using default algorithmn
uint64_t hashing_hash_default(const void* data, size_t size);

// Other option
uint64_t hashing_hash_xxhash(const  void* data, size_t size);
uint64_t hashing_hash_fnv(const void* data, size_t size);
uint64_t hashing_hash_openjdk8(const void* data, size_t size);
uint64_t hashing_hash_test_mode(const void* data, size_t size);

#endif

