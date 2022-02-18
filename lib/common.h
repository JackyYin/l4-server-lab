#ifndef _COMMON_H
#define _COMMON_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIKELY(x) (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))

#define LOG_INFO(...) fprintf(stdout, __VA_ARGS__)
#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)

#define MIN(A, B) ((A) > (B) ? (B) : (A))

#define offsetof(type, element) ((size_t) & ((type *)0x0)->element)

#define container_of(p, type, element)                                         \
    ((type *)((size_t)p - offsetof(type, element)))

#endif
