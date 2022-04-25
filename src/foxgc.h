#ifndef _headers_1645684135_FoxGC_api
#define _headers_1645684135_FoxGC_api

#include <stddef.h>

#define EXPORT_SYM __attribute__((visibility("default"))) 

// Suitable to drop to projects which uses FoxGC
// All user facing API is prefixed with `foxgc_api_` (anything else is internal use)

// Types
typedef struct foxgc_heap                     foxgc_heap_t; 
typedef struct foxgc_descriptor               foxgc_descriptor_t;
typedef struct foxgc_root                     foxgc_root_t;
typedef struct foxgc_object                   foxgc_object_t;
typedef struct foxgc_root_reference           foxgc_root_reference_t;

enum foxgc_version_type {
  FOXGC_VERSION_UNKNOWN,
  FOXGC_VERSION_DEV,
  FOXGC_VERSION_ALPHA,
  FOXGC_VERSION_BETA,
  FOXGC_VERSION_RELEASE 
};
typedef struct foxgc_version {
  enum foxgc_version_type type;
  int patch;
  int minor;
  int major; 
} foxgc_version_t;

// Finalizers
typedef void (^foxgc_finalizer)(foxgc_object_t*);

EXPORT_SYM struct foxgc_object* foxgc_api_new_object(foxgc_heap_t* this, foxgc_root_t* root, foxgc_root_reference_t** rootRef, foxgc_descriptor_t* desc, foxgc_finalizer finalizer);
EXPORT_SYM struct foxgc_object* foxgc_api_new_array(foxgc_heap_t* this, foxgc_root_t* root, foxgc_root_reference_t** rootRef, int arraySize);
EXPORT_SYM struct foxgc_object* foxgc_api_new_data_array(foxgc_heap_t* this, foxgc_root_t* root, foxgc_root_reference_t** rootRef, size_t memberSize, int arraySize);


EXPORT_SYM int foxgc_get_array_size(foxgc_object_t* obj);
EXPORT_SYM size_t foxgc_get_member_size(foxgc_object_t* obj);

EXPORT_SYM struct foxgc_heap* foxgc_api_new(size_t gen0_size,
                                            size_t gen1_size,
                                            size_t gen2_size,
                                            
                                            int gen0_promotionAge,
                                            int gen1_promotionAge,
                                            
                                            size_t gen0_largeObjectSize,
                                            size_t gen1_largeObjectSize,
                                            
                                            size_t metaspace_size);

// Stored in metaspace (essentially space where metadata about 
// which part is containing pointer) cleared when the heap destroyed
// offset is offset to field pointing to foxgc_object 
EXPORT_SYM foxgc_descriptor_t* foxgc_api_descriptor_new(foxgc_heap_t* this, 
                                                        int numPointers, 
                                                        size_t* offsets,
                                                        size_t objectSize);
EXPORT_SYM void foxgc_api_free(foxgc_heap_t* this);

EXPORT_SYM foxgc_root_t* foxgc_api_new_root(foxgc_heap_t* this);
EXPORT_SYM void foxgc_api_delete_root(foxgc_heap_t* this, foxgc_root_t* root);
EXPORT_SYM void foxgc_api_remove_from_root(foxgc_heap_t* this, foxgc_root_t* root, foxgc_object_t* obj);
EXPORT_SYM void foxgc_api_remove_from_root2(foxgc_heap_t* this, foxgc_root_t* root, foxgc_root_reference_t* rootRef);

EXPORT_SYM size_t foxgc_api_get_gen_usage(foxgc_heap_t* this, int id);
EXPORT_SYM size_t foxgc_api_get_gen_size(foxgc_heap_t* this, int id);

EXPORT_SYM size_t foxgc_api_get_heap_size(foxgc_heap_t* this);
EXPORT_SYM size_t foxgc_api_get_heap_usage(foxgc_heap_t* this);
EXPORT_SYM size_t foxgc_api_get_total_reclaimed_bytes(foxgc_heap_t* this);

EXPORT_SYM size_t foxgc_api_get_metaspace_usage(foxgc_heap_t* this);
EXPORT_SYM size_t foxgc_api_get_metaspace_size(foxgc_heap_t* this);

// Pass -1 ID to get statistic for full GC
EXPORT_SYM int foxgc_api_get_gen_number_of_collections(foxgc_heap_t* this, int id);
EXPORT_SYM float foxgc_api_get_gen_total_collection_time(foxgc_heap_t* this, int id);
EXPORT_SYM size_t foxgc_api_get_gen_total_reclaimed_bytes(foxgc_heap_t* this, int id);

EXPORT_SYM int foxgc_api_get_generation_count(foxgc_heap_t* heap);

// Other stuff
EXPORT_SYM void* foxgc_api_object_get_data(foxgc_object_t* obj);
EXPORT_SYM void foxgc_api_do_full_gc(foxgc_heap_t* this);


// Only write that must go through write barrier not read
EXPORT_SYM void foxgc_api_write_field(foxgc_object_t* obj, int index, foxgc_object_t* newValue);
EXPORT_SYM void foxgc_api_write_array(foxgc_object_t* obj, int index, foxgc_object_t* newValue);


// Your implementation name
EXPORT_SYM const char* foxgc_api_get_implementation_name();

// Your implementation version
EXPORT_SYM const foxgc_version_t* foxgc_api_get_implementation_version();

// Specification version you are implementing
EXPORT_SYM const foxgc_version_t* foxgc_api_get_specification_version();

// Return full name. Example: 'MyGC 1.0.0 implements specification version 2.0.0'
EXPORT_SYM const char* foxgc_api_full_version_name();

// For when GC tracked object
// placed in non GC tracked structure
void foxgc_api_increment_ref(foxgc_object_t* obj);
void foxgc_api_decrement_ref(foxgc_object_t* obj);

#endif

