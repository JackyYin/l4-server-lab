#include <stdlib.h>
#include <stdio.h>

#include "coro.h"

void f(coroutine *co, void *data)
{
    fprintf(stdout, "Hello !\n");
    for (int i = 0; i < 100; i++) {
        fprintf(stdout, "%d\n", i);
        co_yield(co, i);
    }
}

int main() {
    char *buf;
    coroutine *co;
    if (!(buf = malloc(128)))
        fprintf(stderr, "failed to alloc for buf...\n");

    if (!(co = co_new(f, (void*) buf)))
        fprintf(stderr, "failed to alloc for coroutine...\n");

    fprintf(stdout, "sizeof co_context: %lu\n", sizeof(co_context));
    fprintf(stdout, "sizeof corountine: %lu\n", sizeof(coroutine));

    /*
     * NOTE: don't resume after the coroutine stop,
     * otherwise it's an undefined behavior!
     */
    while (co->status != CO_STATUS_STOPPED) {
        co_resume(co);
    }
}
