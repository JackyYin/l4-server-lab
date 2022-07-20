#ifndef _IO_HANDLER_H
#define _IO_HANDLER_H

#include "server.h"

static int __prepare_connection(struct server_info *svr, int fd,
                                co_func handler)
{
    struct server_connection *conn = &svr->conns[fd];
    if (UNLIKELY((conn->coro = co_new((co_func)handler, (void *)conn)) ==
                 NULL)) {
        LOG_ERROR("Failed to create coroutine for fd: %d\n", fd);
        return -1;
    }

#ifndef NDEBUG
    LOG_INFO("created coro %p for fd: %d\n", conn->coro, fd);
#endif

    // This fd never accept connection before
    if (UNLIKELY(!conn->str &&
                 (conn->str = string_init(DEFAULT_SVR_BUFLEN)) == NULL)) {
        LOG_ERROR("Failed to init string...\n");
        return -1;
    }
    conn->fd = fd;
    conn->action = 0;
    conn->svr = svr;
    return 0;
}

#endif
