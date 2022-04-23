#include "http_parser.h"

static int str_to_int32(char *buf)
{
    int val = 0;

    memcpy(&val, buf, 4);

    return val;
}

ROUTER(thisisateapot) { return 0; }

void *find_router(const char *path)
{
    const struct router *iter;

    FOREACH_ROUTER(iter)
    {
        if (strcmp(path, iter->path) == 0) {
            return iter->fp;
        }
    }
    return NULL;
}

static void trim_spaces(char **ppbuf, char *bufend)
{
    while (*ppbuf < bufend && (*ppbuf)[0] == ' ')
        (*ppbuf)++;
}

static int http_parse_method(struct http_request *req, char **ppbuf,
                             char *bufend)
{
    if (UNLIKELY(bufend - *ppbuf < 4))
        goto BAD_REQ;

    int reqstrn = str_to_int32(*ppbuf);

    FOR_EACH_HTTP_METHOD(method)
    {
        if (method->strn == reqstrn) {
            req->method = method->mask;
            *ppbuf += method->length;
            return method->length;
        }
    }

BAD_REQ:
    return 0;
}

static int http_parse_path(struct http_request *req, char **ppbuf, char *bufend)
{
    char *start = *ppbuf;

    if (UNLIKELY(bufend - start < sizeof("/ HTTP/0.0\r\n")))
        goto BAD_REQ;

    if (UNLIKELY(start[0] != '/'))
        goto BAD_REQ;

    start++;
    char *end_of_path = strchr(start, ' ');
    if (UNLIKELY(!end_of_path))
        goto BAD_REQ;

    char *query = strchr(start, '?');
    if (query && query < end_of_path) {
        *query = '\0';
        query++;

        while (query && query < end_of_path) {
            char *qkv_seperator = strchr(query, '=');
            if (!qkv_seperator || qkv_seperator >= end_of_path)
                break;
            *qkv_seperator = '\0';

            kv_set_key_value(req->query, query, qkv_seperator + 1);
            char *qkv_end = strchr(qkv_seperator + 1, '&');
            if (!qkv_end || qkv_end >= end_of_path)
                break;
            *qkv_end = '\0';
            query = qkv_end + 1;
        }
    }
    *end_of_path = '\0';

    req->path = start;
    *ppbuf = end_of_path + 1;

    return *ppbuf - start;
BAD_REQ:
    return 0;
}

static int http_parse_protocol(struct http_request *req, char **ppbuf,
                               char *bufend)
{
    char *start = *ppbuf;
    if (UNLIKELY(bufend - start < sizeof("HTTP/0.0\r\n")))
        goto BAD_REQ;

    char *end_of_line = strchr(start, '\r');
    if (UNLIKELY(!end_of_line))
        goto BAD_REQ;

    if (UNLIKELY(str_to_int32(start) != STR_TO_INT32('H', 'T', 'T', 'P')))
        goto BAD_REQ;

    req->protocol = start;
    end_of_line[0] = '\0';
    *ppbuf = end_of_line + 2; // skip \r\n

    return *ppbuf - start;
BAD_REQ:
    return 0;
}

static int http_parse_headers(struct http_request *req, char **ppbuf,
                              char *bufend)
{
    char *start = *ppbuf;
    while (start < bufend) {
        char *end_of_header_line = strchr(start, '\r');
        if (UNLIKELY(!end_of_header_line))
            break;
        // we reach end of header section
        if (UNLIKELY(end_of_header_line - start < sizeof("H: V"))) {
            start = end_of_header_line;
            break;
        }

        char *header_seperator = strchr(start, ':');
        if (LIKELY(header_seperator && header_seperator < end_of_header_line)) {
            *header_seperator = '\0';
            *end_of_header_line = '\0';
            kv_set_key_value(req->headers, start, header_seperator + 2);
        }
        start = end_of_header_line + 2;
    }

    int ret = start - *ppbuf;
    *ppbuf = start;
    return ret;
}

int http_parse_request(struct http_request *req, char *buf, size_t buflen)
{
    char *bufend = buf + buflen;
    http_parse_method(req, &buf, bufend);
    if (UNLIKELY(!req->method))
        goto BAD_REQ;

#ifndef NDEBUG
    LOG_INFO("method: %d\n", req->method);
#endif
    trim_spaces(&buf, bufend);

    http_parse_path(req, &buf, bufend);
    if (UNLIKELY(!req->path))
        goto BAD_REQ;

#ifndef NDEBUG
    LOG_INFO("path: %s\n", req->path);
    LOG_INFO("query: \n");
    {
        struct kv_pair *p = NULL;
        FOREACH_KV(req->query, p)
        {
            LOG_INFO("key: %s, value: %s\n", p->key, p->value);
        }
    }
#endif

    http_parse_protocol(req, &buf, bufend);
    if (UNLIKELY(!req->protocol))
        goto BAD_REQ;

#ifndef NDEBUG
    LOG_INFO("protocol: %s\n", req->protocol);
#endif

    http_parse_headers(req, &buf, bufend);

#ifndef NDEBUG
    LOG_INFO("headers: \n");
    {
        struct kv_pair *p = NULL;
        FOREACH_KV(req->headers, p)
        {
            LOG_INFO("key: %s, value: %s\n", p->key, p->value);
        }
    }
#endif

#ifndef NDEBUG
    LOG_INFO("%.*s\n", (int)(bufend - buf), buf);
#endif
    return 1;

BAD_REQ:
    return 0;
}

int http_compose_response(struct http_request *req, struct http_response *res,
                          char *final)
{
    if (UNLIKELY(!req->protocol))
        goto BAD_REQ;

    char tmp[100] = {0};
    sprintf(tmp, "%s %d OK\n", req->protocol, res->status || 200);
    strcat(final, tmp);

    if (res->headers && res->headers->size > 0) {
        struct kv_pair *p = NULL;
        FOREACH_KV(res->headers, p)
        {
            sprintf(tmp, "%s: %s\n", p->key, p->value);
            strcat(final, tmp);
        }
    }

    sprintf(tmp, "Content-Length: %ld\n", strlen(res->buf));
    strcat(final, tmp);
    strcat(final, "\n");
    strcat(final, res->buf);
    return 1;
BAD_REQ:
    return 0;
}
