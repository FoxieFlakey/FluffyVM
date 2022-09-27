#ifndef header_1662559219_f84c3c01_b630_4c2d_8f68_bccec8cf6a7f_string_h
#define header_1662559219_f84c3c01_b630_4c2d_8f68_bccec8cf6a7f_string_h

#include <stddef.h>

struct vm;

struct string {
  size_t length;
  char actualString[];
};

int string_init(struct vm* F);
void string_cleanup(struct vm* F);

#endif

