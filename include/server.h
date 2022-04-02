#ifndef _SERVER_H
#define _SERVER_H

#include "common.h"
#include "coro.h"
#include "socket.h"
#include "syscall.h"
#include <pthread.h>
#include <stdatomic.h>

#define DEFAULT_SVR_BUFLEN (1024)

struct server_buffer {
    char *buf;
    size_t capacity;
    size_t len;
};

struct server_connection {
    int fd;
    uint32_t action;
    _Atomic uint32_t refcnt;
    coroutine *coro;
    struct server_buffer buf;
};

struct server_info {
    int epoll_fd;
    int listen_fd;

    // flexible array member
    struct server_connection conns[];
};

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

void epoll_listen_loop(pthread_t tid, struct server_info *svr);
void io_uring_listen_loop(pthread_t tid, struct server_info *svr);

struct server_info *create_server(const char *addr, uint16_t port);
void start_listening(struct server_info *svr, int threads_cnt);

#endif
