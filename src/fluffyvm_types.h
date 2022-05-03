#ifndef header_1650878691_fluffyvm_types_h
#define header_1650878691_fluffyvm_types_h

#include "foxgc.h"
#include "value.h"

struct hashtable_static_data { 
  foxgc_descriptor_t* desc_pair;
  foxgc_descriptor_t* desc_hashTable;
};

struct bytecode_static_data { 
  foxgc_descriptor_t* desc_bytecode;
  foxgc_descriptor_t* desc_prototype;
};

#endif

