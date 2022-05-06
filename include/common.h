#ifndef _COMMON_H
#define _COMMON_H

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIKELY(x) (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))

#define LOG_INFO(...) fprintf(stdout, __VA_ARGS__)
#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)

#define offsetof(type, member) ((size_t) & ((type *)0x0)->member)

#define container_of(p, type, member)                                          \
    ((type *)((size_t)p - offsetof(type, member)))

#define __PASTE(A, B) A##B

#define __UNIQUE_ID(prefix) __PASTE(__PASTE(__UNIQUE_ID_, prefix), __COUNTER__)

/*
 * C11 specification for ternary operator:
 * if one operand is a null pointer constant, the result has the type of the
 * other operand. Reference: https://hackmd.io/@sysprog/linux-macro-minmax
 */
#define __is_constexpr(x)                                                      \
    (sizeof(int) == sizeof(*(8 ? ((void *)((long)(x)*0l)) : (int *)8)))

#define __no_side_effects(x, y) (__is_constexpr(x) && __is_constexpr(y))

/*
 *  A compiler warning will be ejected if type of x and y differs
 */
#define __typecheck(x, y) (!!(sizeof((typeof(x) *)1 == (typeof(y) *)1)))

#define __safe_cmp(x, y) (__typecheck(x, y) && __no_side_effects(x, y))

#define __cmp(x, y, op) ((x)op(y) ? (x) : (y))

#define __cmp_once(x, y, unique_x, unique_y, op)                               \
    ({                                                                         \
        typeof(x) unique_x = (x);                                              \
        typeof(y) unique_y = (y);                                              \
        __cmp(unique_x, unique_y, op);                                         \
    })

#define __careful_cmp(x, y, op)                                                \
    __builtin_choose_expr(                                                     \
        __safe_cmp(x, y), __cmp(x, y, op),                                     \
        __cmp_once(x, y, __UNIQUE_ID(__x), __UNIQUE_ID(__y), op))

#define MAX(x, y) __careful_cmp(x, y, >)
#define MIN(x, y) __careful_cmp(x, y, <)

#endif
