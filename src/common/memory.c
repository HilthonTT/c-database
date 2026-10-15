#include "common/memory.h"

#include <stdlib.h>

#include "common/fatal.h"

void* xmalloc(size_t size) {
    void* pointer = malloc(size);
    if (pointer == NULL) {
        fatal("out of memory allocating %zu bytes", size);
    }
    return pointer;
}

void* xcalloc(size_t count, size_t size) {
    void* pointer = calloc(count, size);
    if (pointer == NULL) {
        fatal("out of memory allocating %zu x %zu bytes", count, size);
    }
    return pointer;
}
