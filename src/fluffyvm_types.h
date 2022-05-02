#ifndef header_1650878691_fluffyvm_types_h
#define header_1650878691_fluffyvm_types_h

#include "foxgc.h"
#include "value.h"

struct value_static_data {
  bool hasInit;

  struct value outOfMemoryString;
  foxgc_root_reference_t* outOfMemoryStringRootRef;
  
  struct value outOfMemoryWhileHandlingError;
  foxgc_root_reference_t* outOfMemoryWhileHandlingErrorRootRef;
  
  struct value outOfMemoryWhileAnErrorOccured;
  foxgc_root_reference_t* outOfMemoryWhileAnErrorOccuredRootRef;
  
  struct value strtodDidNotProcessAllTheData;
  foxgc_root_reference_t* strtodDidNotProcessAllTheDataRootRef;

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
  struct value error_invalidCapacity;
  foxgc_root_reference_t* error_invalidCapacityRootRef;
  
  struct value error_badKey;
  foxgc_root_reference_t* error_badKeyRootRef;
  
  foxgc_descriptor_t* desc_pair;
  foxgc_descriptor_t* desc_hashTable;
};

struct bytecode_static_data { 
  foxgc_descriptor_t* desc_bytecode;
  foxgc_descriptor_t* desc_prototype;
};

#endif

