#ifndef _KEY_VALUE_H
#define _KEY_VALUE_H

#include "common.h"

#define DEFAULT_KV_SZ (20)

struct kv_pair {
    char *key;
    char *value;
};

struct kv {
    int capacity;
    int size;
    struct kv_pair *pairs;
};

#define FOREACH_KV(kv, p)                                                      \
    p = kv->pairs;                                                             \
    for (int i = 0; i < kv->size; i++, p++)

struct kv *kv_init();
bool kv_set_key_value(struct kv *kvmap, char *key, char *value);
char *kv_find_value(struct kv *kvmap, const char *key);
void kv_free(struct kv *kvmap);
void kv_reset(struct kv *kvmap);

#endif
