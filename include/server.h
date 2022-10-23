#ifndef _SERVER_H
#define _SERVER_H

#include "common.h"
#include "coro.h"
#include "kv.h"
#include "socket.h"
#include "strbuf.h"
#include "syscall.h"
#include "router.h"
#include <pthread.h>

#define METHOD_GET (1 << 0)
#define METHOD_HEAD (1 << 1)
#define METHOD_POST (1 << 2)
#define METHOD_PUT (1 << 3)
#define METHOD_DELETE (1 << 4)
#define METHOD_CONNECT (1 << 5)
#define METHOD_OPTIONS (1 << 6)
#define METHOD_TRACE (1 << 7)
#define METHOD_PATCH (1 << 8)

#define METHOD_TO_INT(X) METHOD_##X

#define DEFAULT_SVR_BUFLEN (1024)

struct server_buffer {
    char *buf;
    size_t capacity;
    size_t len;
};

struct server_connection {
    int fd;
    uint32_t action;
    int cur_steps;
    int total_steps;
    /* _Atomic uint32_t refcnt; */
    coroutine *coro;
    string_t *str;
    struct server_info *svr;
};

struct server_info {
    int listen_fd;
#ifdef WATCH_STATIC_FILES
    int pipefds[2];
#endif
    void *ring; // actually struct io_uring*

    // flexible array member
    struct server_connection conns[];
};

struct http_request {
    int method;
    char *path;     // should be null-terminated
    char *protocol; // should be null-terminated
    struct server_connection *conn;
    struct kv query;
    struct kv headers;
};

struct http_response {
    int status;
    char *file;
    int file_fd;
    size_t file_sz;
    string_t *str;
    struct kv headers;
};

#define SET_RES_HEADER(k, v) kv_set_key_value(&response->headers, k, v)

#define SET_RES_MIME(v) kv_set_key_value(&response->headers, "Content-Type", v)

#define APPEND_RES_BODY(c, clen) string_append(response->str, c, clen)

void epoll_listen_loop(pthread_t tid, struct server_info *svr);
void io_uring_listen_loop(pthread_t tid, struct server_info *svr);

struct server_info *create_server(const char *addr, uint16_t port);
void start_listening(struct server_info *svr, int threads_cnt);

#endif
