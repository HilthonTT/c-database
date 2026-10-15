#include "common/fatal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void fatal(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fflush(stdout);
    fputs("Fatal: ", stderr);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
    exit(EXIT_FAILURE);
}
