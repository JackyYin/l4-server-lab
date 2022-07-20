#ifndef _KEY_VALUE_H
#define _KEY_VALUE_H

#include "common.h"

#define DEFAULT_KV_SZ (20)

/*
 * KV_FLAG_STATIC means that the inner buffer of pairs is provided by user
 * instead of by malloc, we use 0 to represent it, so we can save the effort
 * of setting flag value when we reset the struct.
 */
#define KV_FLAG_PROVIDED (0)
#define KV_FLAG_MALLOC (1 << 1)

struct kv_pair {
    char *key;
    char *value;
};

struct kv {
    int size;
    int flag;
    int capacity;
    struct kv_pair *pairs;
};

#define KV_INIT_WITH_BUF(buf, len)                                             \
    (struct kv) { .pairs = buf, .capacity = len }

#define FOREACH_KV(kv, p)                                                      \
    p = (kv)->pairs;                                                           \
    for (int i = 0; i < (kv)->size; i++, p++)

struct kv *kv_init();
bool kv_set_key_value(struct kv *kvmap, char *key, char *value);
char *kv_find_value(struct kv *kvmap, const char *key);
void kv_free(struct kv *kvmap);
void kv_reset(struct kv *kvmap);

#endif
