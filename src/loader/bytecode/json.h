#ifndef header_1651549506_json_h
#define header_1651549506_json_h

#include "../../bytecode.h"

bool bytecode_loader_json_init(struct fluffyvm *vm);
void bytecode_loader_json_cleanup(struct fluffyvm *vm);

// JSON loader
// Intended to load embedded bytecode
// Limitations:
//  * Bytecode cannot have strings which have
//    embedded zeros in it
//  * Very slow compare to other methods
struct bytecode* bytecode_loader_json_load(struct fluffyvm* vm, foxgc_root_reference_t** rootRef, const char* buffer, size_t len); 

#endif

