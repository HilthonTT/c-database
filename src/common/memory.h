#ifndef COMMON_MEMORY_H
#define COMMON_MEMORY_H

#include <stddef.h>

void* xmalloc(size_t size);
void* xcalloc(size_t count, size_t size);

#endif
