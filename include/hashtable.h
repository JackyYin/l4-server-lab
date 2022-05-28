/* A reimplementation of hashmap according to Linux Kernel
 *
 * Also reference: https://hackmd.io/@sysprog/linux2022-quiz1
 */

#ifndef _HASH_TABLE_H
#define _HASH_TABLE_H

#include "common.h"

#define GOLDEN_RATIO_32 0x61C88647
#define GOLDEN_RATIO_64 0x61C8864680B583EBull

/* Use hash_32 when possible to allow for fast 32bit hashing in 64bit kernels.
 */
#define hash_min(val, bits)                                                    \
    (sizeof(val) <= 4 ? hash_32(val, bits) : hash_64(val, bits))

__attribute__((unused)) static uint32_t hash_32(uint32_t val, uint8_t bits)
{
    return (val * GOLDEN_RATIO_32) >> (32 - bits);
}

__attribute__((unused)) static uint64_t hash_64(uint64_t val, uint8_t bits)
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

struct hmap *hmap_new(uint8_t bits);

void *hmap_get(struct hmap *hashmap, uint64_t key);

void hmap_insert(struct hmap *hashmap, uint64_t key, void *value);

void hmap_delete(struct hmap *hashmap);

#endif
