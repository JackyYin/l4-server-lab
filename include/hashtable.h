/* A reimplementation of hashmap according to Linux Kernel
 *
 * Also reference: https://hackmd.io/@sysprog/linux2022-quiz1
 */

#ifndef _HASH_TABLE_H
#define _HASH_TABLE_H

#include "common.h"

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
    uint64_t (*hash_func)(const void *key, uint8_t bits);
    // flexible array member
    struct hlist_head hl[]; // head list
};

struct hmap *str_hmap_new(uint8_t bits);

struct hmap *int_hmap_new(uint8_t bits);

void *hmap_get(struct hmap *hashmap, const void *key);

void hmap_insert(struct hmap *hashmap, const void *key, void *value);

void hmap_delete(struct hmap *hashmap);

#endif
