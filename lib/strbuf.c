#include "strbuf.h"

string_t *string_init(size_t capacity)
{
    string_t *str = NULL;
    if (UNLIKELY((str = malloc(sizeof(string_t))) == NULL)) {
        return NULL;
    }

    size_t real_capacity = (1 << __builtin_clzl(capacity));
    real_capacity = MAX(real_capacity, 64UL);
    if (UNLIKELY((str->buf = malloc(real_capacity)) == NULL)) {
        free(str);
        return NULL;
    }

    str->capacity = real_capacity;
    str->len = 0;
    memset(str->buf, 0, real_capacity);
    return str;
}

bool string_append(string_t *str, char *buf, size_t buflen)
{
    if (UNLIKELY(!str))
        return false;

    if (UNLIKELY(str->len + buflen >= str->capacity)) {
        size_t real_capacity = (1 << __builtin_clzl(str->len + buflen));
        char *newb = NULL;
        if (UNLIKELY((newb = realloc(str->buf, real_capacity)) == NULL)) {
            return false;
        }
        str->buf = newb;
        str->capacity = real_capacity;
    }

    strncat(str->buf, buf, buflen);
    str->len += buflen;
    return true;
}

void string_reset(string_t *str)
{
    if (LIKELY(str)) {
        memset(str->buf, 0, str->capacity);
        str->len = 0;
    }
}

void string_free(string_t *str)
{
    if (str)
        free(str);
}
