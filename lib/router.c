#include "common.h"
#include "router.h"

const struct router *find_router(const char *path, int method)
{
    const struct router *iter;

    FOREACH_ROUTER(iter)
    {
        if ((method & iter->method) && strcmp(path, iter->path) == 0) {
            return iter;
        }
    }
    return NULL;
}
