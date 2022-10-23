#include <fcntl.h>
#include <magic.h>

#include "http_parser.h"

static magic_t magic_instance;

static void __attribute__((constructor)) magic_init()
{
    magic_instance = magic_open(MAGIC_MIME_TYPE | MAGIC_CHECK);
    if (magic_instance)
        printf("magic inited....\n");

    if (magic_load(magic_instance, NULL) == 0)
        printf("magic loaded....\n");
}

static void __attribute__((destructor)) magic_uninit()
{
    magic_close(magic_instance);
    printf("magic closed....\n");
}

static int str_to_int32(char *buf)
{
    int val = 0;

    memcpy(&val, buf, 4);

    return val;
}

ROUTER(GET, thisisateapot) { return 0; }

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

    if (UNLIKELY((size_t)(bufend - start) < sizeof("/ HTTP/0.0\r\n")))
        goto BAD_REQ;

    if (UNLIKELY(start[0] != '/'))
        goto BAD_REQ;

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

            kv_set_key_value(&req->query, query, qkv_seperator + 1);
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
    if (UNLIKELY((size_t)(bufend - start) < sizeof("HTTP/0.0\r\n")))
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
        if (UNLIKELY((size_t)(end_of_header_line - start) < sizeof("H: V"))) {
            start = end_of_header_line;
            break;
        }

        char *header_seperator = strchr(start, ':');
        if (LIKELY(header_seperator && header_seperator < end_of_header_line)) {
            *header_seperator = '\0';
            *end_of_header_line = '\0';
            kv_set_key_value(&req->headers, start, header_seperator + 2);
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
        FOREACH_KV(&req->query, p)
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
        FOREACH_KV(&req->headers, p)
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

int global_static_router(__attribute__((unused)) struct http_request *request,
                         struct http_response *response,
                         const char *filepath)
{
    int fd;
    struct stat filestat;
#ifndef WATCH_STATIC_FILES
    static void *data;

    if (data) {
        goto STORE_BUF;
    }
#endif

    if (UNLIKELY((fd = __open(filepath, O_RDONLY)) < 0))
        return HANDLER_ERR_FILE_NOT_FOUND;

    if (UNLIKELY(__fstat(fd, &filestat) < 0)) {
        LOG_INFO("Failed to retrieve file stat...\n");
        __close(fd);
        return HANDLER_ERR_GENERAL;
    }

    const char *mime_str = NULL;
    if (UNLIKELY((mime_str = magic_descriptor(magic_instance, fd)) == NULL)) {
        printf("magic error: %s\n", magic_error(magic_instance));
        return HANDLER_ERR_GENERAL;
    }

#ifndef WATCH_STATIC_FILES
    data = mmap(NULL, filestat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (UNLIKELY(data == MAP_FAILED)) {
        LOG_INFO("Failed to mmap...\n");
        __close(fd);
        return HANDLER_ERR_GENERAL;
    }

STORE_BUF:
    response->file = data;
#else
    response->file_fd = fd;
#endif
    response->file_sz = filestat.st_size;
    response->status = 200;
    SET_RES_MIME((char *)mime_str);
    return HANDLER_ERR_SUCCESS;
}

int http_compose_response(struct http_request *req, struct http_response *res,
                          string_t *final)
{
    if (UNLIKELY(!req->protocol))
        goto BAD_REQ;

    char tmp[1024] = {0};
    int status = res->status ? res->status : 200;
    sprintf(tmp, "%s %d OK\n", req->protocol, status);
    string_append(final, tmp, strlen(tmp));

    if (res->headers.size > 0) {
        struct kv_pair *p = NULL;
        FOREACH_KV(&res->headers, p)
        {
            sprintf(tmp, "%s: %s\n", p->key, p->value);
            string_append(final, tmp, strlen(tmp));
        }
    }

    if (res->file) {
        sprintf(tmp, "Content-Length: %ld\n", res->file_sz);
        string_append(final, tmp, strlen(tmp));
        string_append(final, "\n", 1);
        string_append(final, res->file, res->file_sz);
    } else if (res->file_fd) {
        sprintf(tmp, "Content-Length: %ld\n", res->file_sz);
        string_append(final, tmp, strlen(tmp));
        string_append(final, "\n", 1);
    } else {
        sprintf(tmp, "Content-Length: %ld\n", res->str->len);
        string_append(final, tmp, strlen(tmp));
        string_append(final, "\n", 1);
        string_append(final, res->str->buf, res->str->len);
    }
    return 1;
BAD_REQ:
    return 0;
}
