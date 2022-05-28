#ifndef _STRBUF_H
#define _STRBUF_H

#include "common.h"

typedef struct {
    size_t capacity;
    size_t len;
    char *buf;
} string_t;

string_t *string_init(size_t capacity);

bool string_append(string_t *str, char *buf, size_t buflen);

void string_reset(string_t *str);

void string_free(string_t *str);

#endif
