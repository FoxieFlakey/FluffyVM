#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "auto_cleanups.h"

void auto_free(void** ptr) {
  if (*ptr != NULL)
    free(*ptr);
  
  *ptr = NULL;
}

void auto_free_char_buffer(char** ptr) {
  if (*ptr != NULL)
    free(*ptr);
  
  *ptr = NULL;
}

void auto_fclose(FILE** ptr) {
  if (*ptr != NULL)
    fclose(*ptr);
  
  *ptr = NULL;
}

void auto_close(int* ptr) {
  close(*ptr);
}
