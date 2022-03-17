#ifndef _SERVER_H
#define _SERVER_H

#include "common.h"
#include "coro.h"
#include "socket.h"
#include <stdatomic.h>

#define DEFAULT_SVR_BUFLEN (1024)

#define QD (1024)

#define IO_URING_OP_ACCEPT (1)
#define IO_URING_OP_READ (2)
#define IO_URING_OP_WRITE (3)

// this structure can't be larger than 64 bytes
struct io_uring_user_data {
    void *ptr; // point to connection
    int op;
};

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
