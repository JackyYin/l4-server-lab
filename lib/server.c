#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <unistd.h>

#include "server.h"

static int64_t get_nofile_limit()
{
    struct rlimit rlim;

    if (UNLIKELY(getrlimit(RLIMIT_NOFILE, &rlim) < 0)) {
        LOG_ERROR("Failed to get rlimit: %s\n", strerror(errno));
        return -1;
    }

    LOG_INFO("nofile soft limit: %lu\n", rlim.rlim_cur);
    LOG_INFO("nofile hard limit: %lu\n", rlim.rlim_max);

    return rlim.rlim_cur;
}

struct server_info *create_server(const char *addr, uint16_t port)
{
    int64_t nfds;
    struct server_info *svr;

    if (UNLIKELY((nfds = get_nofile_limit()) < 0)) {
        LOG_ERROR("Failed to get max fd count\n");
        goto EXIT;
    }

    if (UNLIKELY((svr = malloc(sizeof(struct server_info) +
                               sizeof(struct server_connection) * nfds)) ==
                 NULL)) {
        LOG_ERROR("Failed to allocate\n");
        goto EXIT;
    }

    if (UNLIKELY((svr->epoll_fd = epoll_create1(EPOLL_CLOEXEC)) == -1)) {
        LOG_ERROR("Failed to create epoll instance: %s\n", strerror(errno));
        goto FREE;
    }

    if (UNLIKELY((svr->listen_fd = create_listening_socket(addr, port)) < 0)) {
        goto CLOSE_EPOLL;
    }

    return svr;
// fall through
CLOSE_EPOLL:
    close(svr->epoll_fd);

// fall through
FREE:
    free(svr);

EXIT:
    return NULL;
}
