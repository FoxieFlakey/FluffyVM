#ifndef header_1650878691_fluffyvm_types_h
#define header_1650878691_fluffyvm_types_h

#include "foxgc.h"
#include "value.h"

struct value_static_data {
  struct value outOfMemoryString;
  foxgc_root_reference_t* outOfMemoryStringRootRef;

  // Type names
  struct {
    struct value nil;
    foxgc_root_reference_t* nilRootRef;
    
    struct value longNum;
    foxgc_root_reference_t* longNumRootRef;
    
    struct value doubleNum;
    foxgc_root_reference_t* doubleNumRootRef;
    
    struct value string;
    foxgc_root_reference_t* stringRootRef;
    
    struct value table;
    foxgc_root_reference_t* tableRootRef;
  } typenames;
};

struct hashtable_static_data {
  foxgc_descriptor_t* desc_pair;
  foxgc_descriptor_t* desc_hashTable;
};

#endif

