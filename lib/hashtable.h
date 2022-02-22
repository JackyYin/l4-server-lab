/* A reimplementation of hashmap according to Linux Kernel
 *
 * Also reference: https://hackmd.io/@sysprog/linux2022-quiz1
 */

#ifndef _HASH_TABLE_H
#define _HASH_TABLE_H

#include "common.h"
#include <stdlib.h>

#define GOLDEN_RATIO_32 0x61C88647
#define GOLDEN_RATIO_64 0x61C8864680B583EBull

/* Use hash_32 when possible to allow for fast 32bit hashing in 64bit kernels.
 */
#define hash_min(val, bits)                                                    \
    (sizeof(val) <= 4 ? hash_32(val, bits) : hash_64(val, bits))

static uint32_t hash_32(uint32_t val, uint8_t bits)
{
    return (val * GOLDEN_RATIO_32) >> (32 - bits);
}

static uint64_t hash_64(uint64_t val, uint8_t bits)
{
    return (val * GOLDEN_RATIO_64) >> (64 - bits);
}

struct hlist_head {
    struct hlist_node *first;
};

struct hlist_node {
    struct hlist_node *next, **pprev;
};

struct hmap_element {
    uint64_t key;
    void *value;
    struct hlist_node node;
};

struct hmap {
    uint8_t bits;
    // flexible array member
    struct hlist_head hl[]; // head list
};

struct hmap *hmap_new(uint8_t bits)
{
    struct hmap *hashmap = NULL;
    size_t hllen = sizeof(struct hlist_head) * (1 << bits);
    if (UNLIKELY((hashmap = malloc(sizeof(struct hmap) + hllen)) == NULL))
        return NULL;

    hashmap->bits = bits;
    memset(hashmap->hl, 0, hllen);
    /* for (int i = 0; i < (1 << bits); i++) { */
    /*     hashmap->hl[i].first = NULL; */
    /* } */
    return hashmap;
}

static struct hmap_element *hmap_find(struct hmap *hashmap, uint64_t key)
{
    struct hlist_head *head = &hashmap->hl[hash_min(key, hashmap->bits)];
    struct hmap_element *ele = NULL;
    for (struct hlist_node *pnode = head->first; pnode != NULL;
         pnode = pnode->next) {
        ele = container_of(pnode, struct hmap_element, node);
        if (ele->key == key)
            return ele;
    }
    return NULL;
}

void *hmap_get(struct hmap *hashmap, uint64_t key)
{
    struct hmap_element *ele = hmap_find(hashmap, key);
    return ele ? ele->value : NULL;
}

void hmap_insert(struct hmap *hashmap, uint64_t key, void *value)
{
    struct hmap_element *ele = hmap_find(hashmap, key);
    if (ele)
        return;

    if (UNLIKELY((ele = malloc(sizeof(struct hmap_element))) == NULL))
        return;

    ele->key = key;
    ele->value = value;

    struct hlist_node *new_node = &ele->node;
    struct hlist_head *h = &hashmap->hl[hash_min(key, hashmap->bits)];

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
            free(ele->value);
            free(ele);
        }
    }
    free(hashmap);
}

#endif
