#ifndef _COMMON_H
#define _COMMON_H

#define LIKELY(x)   (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))
#endif
