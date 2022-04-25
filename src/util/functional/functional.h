#ifndef _headers_1642663691_MinecraftLauncherCLI_v2_functionals
#define _headers_1642663691_MinecraftLauncherCLI_v2_functionals

#include <stdbool.h>
#include "../../collections/list.h"

// Some useful functional interfaces from Java
// Link if you are reading this. Its just these really useful in most languages
// yes i know i can C++ if go to C++ i would go Java instead

// Supplier
typedef void* (^supplier_t)();

// Consumer (false if failed, true if success)
typedef bool (^consumer_t)(void* arg1);
typedef bool (^biconsumer_t)(void* arg1, void* arg2);
typedef bool (^triconsumer_t)(void* arg1, void* arg2, void* arg3);

// Runnable
typedef void (^runnable_t)();

supplier_t functional_from_list_iterator(list_iterator_t* list);

#endif

