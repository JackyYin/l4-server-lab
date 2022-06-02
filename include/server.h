#ifndef _SERVER_H
#define _SERVER_H

#include "common.h"
#include "coro.h"
#include "kv.h"
#include "socket.h"
#include "strbuf.h"
#include "syscall.h"
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
    /* _Atomic uint32_t refcnt; */
    coroutine *coro;
    string_t *str;
    struct server_info *svr;
};

struct server_info {
    int listen_fd;
    int epoll_fd;
    void *ring; // actually struct io_uring*

    // flexible array member
    struct server_connection conns[];
};

struct http_request {
    int method;
    char *path;     // should be null-terminated
    char *protocol; // should be null-terminated
    struct kv query;
    struct kv headers;
};

struct http_response {
    int status;
    char *file;
    size_t file_sz;
    string_t *str;
    struct kv headers;
};

struct router {
    const char *path;
    const char *filepath;
    void *fp;
    uint16_t method;
} __attribute__((aligned(32)));

#define ROUTER_SYMBOL_NAME(name) #name

#define ROUTER_SYMBOL_START(name) __start_##name

#define ROUTER_SYMBOL_STOP(name) __stop_##name

#define FOREACH_ROUTER_SEC(secname, iter)                                      \
    extern const struct router ROUTER_SYMBOL_START(secname)[];                 \
    extern const struct router ROUTER_SYMBOL_STOP(secname)[];                  \
    for (iter = ROUTER_SYMBOL_START(secname);                                  \
         iter < ROUTER_SYMBOL_STOP(secname);                                   \
         iter = (const struct router *)((uintptr_t)iter + 32))

#define FOREACH_ROUTER(iter) FOREACH_ROUTER_SEC(ROUTER, iter)

/*
 * We force macro expansion of __COUNTER__ from _ROUTER to __ROUTER
 * */
#define __ROUTER(name, m, p)                                                   \
    static int ROUTER##name(struct http_request *, struct http_response *);    \
    __attribute__((                                                            \
        used, section(ROUTER_SYMBOL_NAME(                                      \
                  ROUTER)))) static const struct router ROUTER##name##info = { \
        .method = METHOD_TO_INT(m), .path = #p, .fp = ROUTER##name};           \
    static int ROUTER##name(                                                   \
        __attribute__((unused)) struct http_request *request,                  \
        __attribute__((unused)) struct http_response *response)
#define _ROUTER(name, method, path) __ROUTER(name, method, path)
#define ROUTER(method, path) _ROUTER(__COUNTER__, method, path)

int global_static_router(__attribute__((unused)) struct http_request *request,
                         __attribute__((unused)) struct http_response *response,
                         const char *filepath);

#define __ROUTER_STATIC(name, m, filep, p)                                     \
    __attribute__((                                                            \
        used, section(ROUTER_SYMBOL_NAME(                                      \
                  ROUTER)))) static const struct router ROUTER##name##info = { \
        .path = #p,                                                            \
        .filepath = #filep,                                                    \
        .fp = global_static_router,                                            \
        .method = METHOD_TO_INT(m)};
#define _ROUTER_STATIC(name, filepath, path)                                   \
    __ROUTER_STATIC(name, GET, filepath, path)
#define ROUTER_STATIC(filepath, path)                                          \
    _ROUTER_STATIC(__COUNTER__, filepath, path)

#define SET_RES_HEADER(k, v) kv_set_key_value(&response->headers, k, v)

#define SET_RES_MIME(v) kv_set_key_value(&response->headers, "Content-Type", v)

#define APPEND_RES_BODY(c, clen) string_append(response->str, c, clen)

void epoll_listen_loop(pthread_t tid, struct server_info *svr);
void io_uring_listen_loop(pthread_t tid, struct server_info *svr);

struct server_info *create_server(const char *addr, uint16_t port);
void start_listening(struct server_info *svr, int threads_cnt);

#endif
