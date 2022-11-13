#ifndef _headers_1667482340_CProjectTemplate_bug
#define _headers_1667482340_CProjectTemplate_bug

#include <stdio.h>
#include <stdlib.h>

// Linux kernel style bug 

#define BUG() do { \
 fprintf(stderr, "BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
 abort(); \
} while(0)

#define BUG_ON(cond) do { \
  if (cond)  \
    BUG(); \
} while(0)

#endif




