#ifndef _ROUTER_H
#define _ROUTER_H

#define ROUTER_SIZE (32)

struct router {
    const char *path;
    const char *filepath;
    void *fp;
    uint16_t method;
} __attribute__((aligned(ROUTER_SIZE)));

#define ROUTER_SECTION_NAME(name) #name

/*
 * We force macro expansion of __COUNTER__ from _ROUTER to __ROUTER
 * */
#define __ROUTER(name, m, p)                                                   \
    static int ROUTER##name(struct http_request *, struct http_response *);    \
    __attribute__((                                                            \
        used, section(ROUTER_SECTION_NAME(                                     \
                  ROUTER)))) static const struct router ROUTER##name##info = { \
        .method = METHOD_TO_INT(m), .path = #p, .fp = ROUTER##name};           \
    static int ROUTER##name(                                                   \
        __attribute__((unused)) struct http_request *request,                  \
        __attribute__((unused)) struct http_response *response)
#define _ROUTER(name, method, path) __ROUTER(name, method, path)
#define ROUTER(method, path) _ROUTER(__COUNTER__, method, path)

struct http_request;
struct http_response;
int global_static_router(__attribute__((unused)) struct http_request *request,
                         __attribute__((unused)) struct http_response *response,
                         const char *filepath);

#define __ROUTER_STATIC(name, m, filep, p)                                     \
    __attribute__((                                                            \
        used, section(ROUTER_SECTION_NAME(                                     \
                  ROUTER)))) static const struct router ROUTER##name##info = { \
        .path = #p,                                                            \
        .filepath = #filep,                                                    \
        .fp = global_static_router,                                            \
        .method = METHOD_TO_INT(m)};
#define _ROUTER_STATIC(name, filepath, path)                                   \
    __ROUTER_STATIC(name, GET, filepath, path)
#define ROUTER_STATIC(filepath, path)                                          \
    _ROUTER_STATIC(__COUNTER__, filepath, path)

extern const struct router __start_ROUTER;
extern const struct router __stop_ROUTER;

extern struct router *router_start;
extern struct router *router_end;

#define ROUTER_SEC_STORE()                                                     \
    struct router *router_start = (struct router *)&__start_ROUTER;            \
    struct router *router_end = (struct router *)&__stop_ROUTER;

const struct router *find_router(const char *path, int method);

#define FOREACH_ROUTER(iter)                                                   \
    for (iter = router_start; iter < router_end;                               \
         iter = (const struct router *)((uintptr_t)iter + ROUTER_SIZE))

#endif
