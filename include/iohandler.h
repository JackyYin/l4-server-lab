#ifndef _IO_HANDLER_H
#define _IO_HANDLER_H

#include "server.h"

static int __prepare_connection(pthread_t tid, int fd,
                                struct server_connection *conn, co_func handler)
{
    if (UNLIKELY((conn->coro = co_new((co_func)handler, (void *)conn)) ==
                 NULL)) {
        LOG_ERROR("[%lu] Failed to create coroutine for fd: %d\n", tid, fd);
        return -1;
    } else {
#ifndef NDEBUG
        LOG_INFO("[%lu] created coro %p for fd: %d\n", tid, conn->coro, fd);
#endif
    }

    // This fd never accept connection before
    if (!conn->buf.buf) {
        if (UNLIKELY((conn->buf.buf = malloc(DEFAULT_SVR_BUFLEN)) == NULL)) {
            conn->buf.capacity = 0;
        } else {
            conn->buf.capacity = DEFAULT_SVR_BUFLEN;
        }
    }
    conn->fd = fd;
    conn->action = 0;
    return 0;
}

#endif
