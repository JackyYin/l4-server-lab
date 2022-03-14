#ifndef _SERVER_H
#define _SERVER_H

#include "common.h"
#include "coro.h"
#include "socket.h"
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

struct server_info *create_server(const char *addr, uint16_t port);

void start_listening(struct server_info *svr, int threads_cnt);

#endif
