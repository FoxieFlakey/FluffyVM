#ifndef _headers_1642395041_MinecraftLauncherCLI_v2_auto_cleanups
#define _headers_1642395041_MinecraftLauncherCLI_v2_auto_cleanups

#include <stdio.h>

// Include useful function for auto closings

void auto_free(void** ptr);
void auto_free_char_buffer(char** ptr);
void auto_fclose(FILE** ptr);
void auto_close(int* ptr);

#define AUTO_CLEAN(func) __attribute__((cleanup(func)))

#endif

