#ifndef header_1662559219_f84c3c01_b630_4c2d_8f68_bccec8cf6a7f_string_h
#define header_1662559219_f84c3c01_b630_4c2d_8f68_bccec8cf6a7f_string_h

#include <FluffyGC/v1.h>
#include <stddef.h>

struct vm;

struct string_subsystem_data {
  fluffygc_descriptor* desc;
};

struct string {
  size_t length;
  
  /* Its opaque object */
  fluffygc_object* actualString; 
};

struct string_gcobject {};

int string_init(struct vm* F);
void string_cleanup(struct vm* F);

struct string_gcobject* string_new_string(struct vm* F, const char* string, size_t len);
struct string_gcobject* string_from_cstring(struct vm* F, const char* string);

const char* string_get_critical(struct vm* F, struct string_gcobject* self);
void string_release_critical(struct vm* F, struct string_gcobject* self, const char* ptr);

#endif

