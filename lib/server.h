#ifndef _SERVER_H
#define _SERVER_H

#include "common.h"
#include "coro.h"
#include "socket.h"

struct server_connection {
    int fd;
    uint32_t action;
    coroutine *coro;
    char *buf;
    size_t capacity;
    size_t len;
};

struct server_info {
    int epoll_fd;
    int listen_fd;

    // flexible array member
    struct server_connection conns[];
};

struct server_info *create_server(const char *addr, uint16_t port);

void start_listening(struct server_info *svr);

#endif
