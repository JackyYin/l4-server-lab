#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#define LIKELY(x)   (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))

#define LOG_INFO(...)  fprintf(stdout, __VA_ARGS__)
#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)

#define MIN(A,B) ((A) > (B) ? (B) : (A))

#endif
