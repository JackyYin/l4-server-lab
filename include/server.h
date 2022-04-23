#ifndef _SERVER_H
#define _SERVER_H

#include "common.h"
#include "coro.h"
#include "kv.h"
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

struct http_request {
    int method;
    char *path;     // should be null-terminated
    char *protocol; // should be null-terminated
    struct kv *query;
    struct kv *headers;
};

struct http_response {
    int status;
    struct kv *headers;
    char buf[1024];
};

typedef int(route_handler)(struct http_request *, struct http_response *);

struct router {
    const char *path;
    route_handler *fp;
};

#define ROUTER_SYMBOL_NAME(name) #name

#define ROUTER_SYMBOL_START(name) __start_##name

#define ROUTER_SYMBOL_STOP(name) __stop_##name

#define FOREACH_ROUTER_SEC(secname, iter)                                      \
    extern const struct router ROUTER_SYMBOL_START(secname)[];                 \
    extern const struct router ROUTER_SYMBOL_STOP(secname)[];                  \
    for (iter = ROUTER_SYMBOL_START(secname);                                  \
         iter < ROUTER_SYMBOL_STOP(secname); iter++)

#define FOREACH_ROUTER(iter) FOREACH_ROUTER_SEC(ROUTER, iter)

#define ROUTER(name)                                                           \
    static int ROUTER##name(struct http_request *, struct http_response *);    \
    __attribute__((                                                            \
        used, section(ROUTER_SYMBOL_NAME(                                      \
                  ROUTER)))) static const struct router ROUTER##name##info = { \
        .path = #name, .fp = ROUTER##name};                                    \
    static int ROUTER##name(struct http_request *request,                      \
                            struct http_response *response)

#define SET_RESPONSE_HEADER(k, v) kv_set_key_value(response->headers, k, v)

#define SET_RESPONSE_MIME(v)                                                   \
    kv_set_key_value(response->headers, "Content-Type", v)

void epoll_listen_loop(pthread_t tid, struct server_info *svr);
void io_uring_listen_loop(pthread_t tid, struct server_info *svr);

struct server_info *create_server(const char *addr, uint16_t port);
void start_listening(struct server_info *svr, int threads_cnt);

#endif
