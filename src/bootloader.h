#ifndef header_1651924866_bootloader_h
#define header_1651924866_bootloader_h

#include <stddef.h>

// Seperated because some editors are struggling
// with very long single line

extern const char* data_bootloader;

// With zero terminator
size_t data_bootloader_get_len();

#endif

