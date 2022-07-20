#include "kv.h"

struct kv *kv_init()
{
    struct kv *kvmap = NULL;
    if (UNLIKELY((kvmap = malloc(sizeof(struct kv))) == NULL))
        return NULL;

    if (UNLIKELY((kvmap->pairs = malloc(sizeof(struct kv_pair) *
                                        DEFAULT_KV_SZ)) == NULL)) {
        free(kvmap);
        return NULL;
    }

    kvmap->capacity = DEFAULT_KV_SZ;
    kvmap->size = 0;
    kvmap->flag = KV_FLAG_MALLOC;
    return kvmap;
}

bool kv_set_key_value(struct kv *kvmap, char *key, char *value)
{
    if (UNLIKELY(!kvmap))
        return 0;

    if (UNLIKELY(kvmap->size == kvmap->capacity)) {
        if ((kvmap->flag & KV_FLAG_MALLOC)) {
            void *newbuf = NULL;
            if (UNLIKELY((newbuf = realloc(kvmap->pairs,
                                           sizeof(struct kv_pair) *
                                               kvmap->capacity * 2)) == NULL)) {
                return 0;
            }
            kvmap->pairs = (struct kv_pair *)newbuf;
            kvmap->capacity *= 2;
        } else {
            return 0;
        }
    }

    kvmap->pairs[kvmap->size++] = (struct kv_pair){.key = key, .value = value};
    return 1;
}

char *kv_find_value(struct kv *kvmap, const char *key)
{
    if (UNLIKELY(!kvmap))
        return NULL;

    struct kv_pair *p = NULL;
    FOREACH_KV(kvmap, p)
    {
        if (strcmp(p->key, key) == 0)
            return p->value;
    }

    return NULL;
}

void kv_free(struct kv *kvmap)
{
    if (LIKELY(kvmap && (kvmap->flag & KV_FLAG_MALLOC))) {
        free(kvmap->pairs);
        free(kvmap);
    }
}

void kv_reset(struct kv *kvmap)
{
    if (LIKELY(kvmap)) {
        memset(kvmap->pairs, 0, kvmap->size * sizeof(struct kv_pair));
        kvmap->size = 0;
    }
}
