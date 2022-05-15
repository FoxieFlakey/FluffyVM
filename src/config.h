#ifndef header_1650978545_config_h
#define header_1650978545_config_h

// Release number
// increment by one for every release
#define FLUFFYVM_RELEASE_NUM (1)

// Call stack stack
#define FLUFFYVM_CALL_STACK_SIZE (512)
#define FLUFFYVM_GENERAL_STACK_SIZE (1024)

#define FLUFFYVM_REGISTERS_NUM (1 << 16)

////////////////////////////////////////
// Hashing config                     //
////////////////////////////////////////

////////////////////////////////////////
// xxHash options                     //
////////////////////////////////////////
// https://github.com/Cyan4973/xxHash
#define FLUFFYVM_HASH_USE_XXHASH
#define FLUFFYVM_XXHASH_SEED (0x55775520)


////////////////////////////////////////
// fnv options                        //
////////////////////////////////////////
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
//#define FLUFFYVM_HASH_USE_FNV
#define FLUFFYVM_FNV_OFFSET_BASIS (0xCBF29CE484222325)
#define FLUFFYVM_FNV_PRIME (0x00000100000001B3)


////////////////////////////////////////
// OpenJDK 8 hash options             //
////////////////////////////////////////
// https://hg.openjdk.java.net/jdk8/jdk8/jdk/file/687fd7c7986d/src/share/classes/java/lang/String.java#l1452 
//
// Refer to linked OpenJDK source code for
// documentation
//#define FLUFFYVM_HASH_USE_OPENJDK8

////////////////////////////////////////
// Test mode hash options             //
////////////////////////////////////////
// Intentional weak hashing
// DO NOT USE THIS IN PRODUCTION
//#define FLUFFYVM_HASH_USE_TEST_MODE

////////////////////////////////////////
// Config validity check here         //
////////////////////////////////////////

#ifdef FLUFFYVM_HASH_USE_XXHASH
# ifdef FLUFFYVM_INTERNAL_HASH_ALGO_SELECTED
#   define FLUFFYVM_ERROR_MULTIPLE_HASH_ALGO_SELECTED
# else
#   define FLUFFYVM_INTERNAL_HASH_ALGO_SELECTED
# endif
#endif

#ifdef FLUFFYVM_HASH_USE_OPENJDK8
# ifdef FLUFFYVM_INTERNAL_HASH_ALGO_SELECTED
#   define FLUFFYVM_ERROR_MULTIPLE_HASH_ALGO_SELECTED
# else
#   define FLUFFYVM_INTERNAL_HASH_ALGO_SELECTED
# endif
#endif

#ifdef FLUFFYVM_HASH_USE_FNV
# ifdef FLUFFYVM_INTERNAL_HASH_ALGO_SELECTED
#   define FLUFFYVM_ERROR_MULTIPLE_HASH_ALGO_SELECTED
# else
#   define FLUFFYVM_INTERNAL_HASH_ALGO_SELECTED
# endif
#endif

#ifdef FLUFFYVM_HASH_USE_TEST_MODE
# ifdef FLUFFYVM_INTERNAL_HASH_ALGO_SELECTED
#   define FLUFFYVM_ERROR_MULTIPLE_HASH_ALGO_SELECTED
# else
#   define FLUFFYVM_INTERNAL_HASH_ALGO_SELECTED
# endif
#endif

#ifdef FLUFFYVM_ERROR_MULTIPLE_HASH_ALGO_SELECTED
# error "Multiple hash algorithmn selected. Please select one!"
#endif

#ifndef FLUFFYVM_INTERNAL_HASH_ALGO_SELECTED
# error "No hash algorithmn selected. Please select one!"
#endif

#undef FLUFFYVM_INTERNAL_HASH_ALGO_SELECTED

#endif

