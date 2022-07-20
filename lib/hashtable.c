/* A reimplementation of hashmap according to Linux Kernel
 *
 * Also reference: https://hackmd.io/@sysprog/linux2022-quiz1
 */

#include "hashtable.h"
#include "murmurhash3.h"
#include <stdlib.h>

#define GOLDEN_RATIO_64 0x61C8864680B583EBull

const uint32_t seed_value = 0x04b91209;

static uint64_t hash_64(const void *key, uint8_t bits)
{
    return ((uint64_t)key * GOLDEN_RATIO_64) >> (64 - bits);
}

static uint64_t murmurhash_str(const void *key,
                               __attribute__((unused)) uint8_t bits)
{
#ifndef __x86_64__
#error Only x86_64 is supported.
#endif
    size_t len = strlen((char *)key);
    uint64_t hash[2];
    MurmurHash3_x64_128(key, len, seed_value, hash);

    printf("%08X\n", hash[0]);
    printf("%08X\n", hash[1]);
    return hash[1];
}

static struct hmap *hmap_new(uint8_t bits,
                             uint64_t (*hash_func)(const void *, uint8_t))
{
    struct hmap *hashmap = NULL;
    size_t hllen = sizeof(struct hlist_head) * (1 << bits);
    if (UNLIKELY((hashmap = malloc(sizeof(struct hmap) + hllen)) == NULL))
        return NULL;

    hashmap->bits = bits;
    hashmap->hash_func = hash_func;
    for (int i = 0; i < (1 << bits); i++) {
        hashmap->hl[i].first = NULL;
    }
    return hashmap;
}

struct hmap *str_hmap_new(uint8_t bits) { return hmap_new(bits, hash_64); }

struct hmap *int_hmap_new(uint8_t bits)
{
    return hmap_new(bits, murmurhash_str);
}

static struct hmap_element *hmap_find(struct hmap *hashmap, const void *key)
{
    struct hlist_head *head =
        &hashmap->hl[hashmap->hash_func(key, hashmap->bits)];
    struct hmap_element *ele = NULL;
    for (struct hlist_node *pnode = head->first; pnode != NULL;
         pnode = pnode->next) {
        ele = container_of(pnode, struct hmap_element, node);
        if (ele->key == key)
            return ele;
    }
    return NULL;
}

void *hmap_get(struct hmap *hashmap, const void *key)
{
    struct hmap_element *ele = hmap_find(hashmap, key);
    return ele ? ele->value : NULL;
}

void hmap_insert(struct hmap *hashmap, const void *key, void *value)
{
    struct hmap_element *ele = hmap_find(hashmap, key);
    if (ele)
        return;

    if (UNLIKELY((ele = malloc(sizeof(struct hmap_element))) == NULL))
        return;

    ele->key = key;
    ele->value = value;

    struct hlist_node *new_node = &ele->node;
    struct hlist_head *h = &hashmap->hl[hashmap->hash_func(key, hashmap->bits)];

    new_node->next = h->first;
    if (h->first) {
        h->first->pprev = &new_node->next;
    }
    h->first = new_node;
    new_node->pprev = &h->first;
}

void hmap_delete(struct hmap *hashmap)
{
    for (int i = 0; i < (1 << hashmap->bits); i++) {
        struct hlist_head *h = &hashmap->hl[i];
        for (struct hlist_node *n = h->first; n;) {
            struct hmap_element *ele =
                container_of(n, struct hmap_element, node);
            n = n->next;
            free(ele);
        }
    }
    free(hashmap);
}
