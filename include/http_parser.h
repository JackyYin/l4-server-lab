#ifndef _HTTP_PARSER_H
#define _HTTP_PARSER_H

#include "hashtable.h"
#include "server.h"

#define METHOD_GET (1 << 0)
#define METHOD_HEAD (1 << 1)
#define METHOD_POST (1 << 2)
#define METHOD_PUT (1 << 3)
#define METHOD_DELETE (1 << 4)
#define METHOD_CONNECT (1 << 5)
#define METHOD_OPTIONS (1 << 6)
#define METHOD_TRACE (1 << 7)
#define METHOD_PATCH (1 << 8)

#define STR_TO_INT32(a, b, c, d)                                               \
    (int)((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

static struct {
    int strn;
    int mask;
    int length;
} methods[] = {
    {.strn = STR_TO_INT32('G', 'E', 'T', ' '), .mask = METHOD_GET, .length = 3},
    {.strn = STR_TO_INT32('P', 'O', 'S', 'T'),
     .mask = METHOD_POST,
     .length = 4},
    {.strn = STR_TO_INT32('P', 'U', 'T', ' '), .mask = METHOD_PUT, .length = 3},
    {.strn = STR_TO_INT32('D', 'E', 'L', 'E'),
     .mask = METHOD_DELETE,
     .length = 6},
    {.strn = STR_TO_INT32('P', 'A', 'T', 'C'),
     .mask = METHOD_PATCH,
     .length = 5},
    {.strn = STR_TO_INT32('H', 'E', 'A', 'D'),
     .mask = METHOD_HEAD,
     .length = 4},
    {.strn = STR_TO_INT32('O', 'P', 'T', 'I'),
     .mask = METHOD_OPTIONS,
     .length = 7},
    {.strn = STR_TO_INT32('T', 'R', 'A', 'C'),
     .mask = METHOD_TRACE,
     .length = 5},
    {.strn = STR_TO_INT32('C', 'O', 'N', 'N'),
     .mask = METHOD_CONNECT,
     .length = 6}};

#define FOR_EACH_HTTP_METHOD(m)                                                \
    typeof(methods[0]) *end = (typeof(methods[0]) *)&methods[0] +              \
                              (sizeof(methods) / sizeof(methods[0]));          \
    for (typeof(methods[0]) *m = &methods[0]; m < end; m++)

#define RESPONSE_200 "HTTP/1.1 200 OK\r\n"
#define RESPONSE_400 "HTTP/1.1 400 Bad Request\r\n"
#define RESPONSE_404 "HTTP/1.1 404 Not Found\r\n"
#define RESPONSE_413 "HTTP/1.1 413 Request Entity Too Large\r\n"

int http_parse_request(struct http_request *req, char *buf, size_t buflen);

void *find_router(const char *path);

int http_compose_response(struct http_request *, struct http_response *,
                          char *);

#endif
