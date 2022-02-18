#include "hashtable.h"

int main()
{
    struct hmap *hashmap;
    if ((hashmap = hmap_new(10)) == NULL)
        return -1;

    for (int i = 0; i < 1000; i++) {
        uint64_t *n = malloc(sizeof(uint64_t));
        *n = i;
        hmap_insert(hashmap, i, (void *)n);
    }

    void *res;
    for (int i = 0; i < 1000; i++) {
        if ((res = hmap_get(hashmap, i)) != NULL) {
            printf("%d\n", (int)*(uint64_t *)res);
        }
    }

    hmap_delete(hashmap);
}
