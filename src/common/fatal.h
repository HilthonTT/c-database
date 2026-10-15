#ifndef COMMON_FATAL_H
#define COMMON_FATAL_H

#if defined(__GNUC__) || defined(__clang__)
#define FATAL_ATTRIBUTES __attribute__((noreturn, format(printf, 1, 2)))
#else
#define FATAL_ATTRIBUTES
#endif

FATAL_ATTRIBUTES void fatal(const char* format, ...);

#endif
