#include <pthread.h>

#include "server.h"

static int64_t get_nofile_limit()
{
    struct rlimit rlim;

    if (UNLIKELY(__getrlimit(RLIMIT_NOFILE, &rlim) < 0)) {
        LOG_ERROR("Failed to get rlimit\n");
        return -1;
    }

    rlim.rlim_cur = rlim.rlim_max;
    if (UNLIKELY(__setrlimit(RLIMIT_NOFILE, &rlim) < 0)) {
        LOG_ERROR("Failed to set rlimit\n");
        return -1;
    }
#ifndef NDEBUG
    LOG_INFO("nofile soft limit: %lu\n", rlim.rlim_cur);
    LOG_INFO("nofile hard limit: %lu\n", rlim.rlim_max);
#endif
    return rlim.rlim_cur;
}

static void *listen_routine(void *arg)
{
    io_uring_listen_loop(pthread_self(), (struct server_info *)arg);
    return NULL;
}

void start_listening(struct server_info *svr, int threads_cnt)
{
    pthread_t *threads;
    if (threads_cnt > 0) {

        if (UNLIKELY((threads = malloc(sizeof(pthread_t) * threads_cnt)) ==
                     NULL)) {
            return;
        }

        for (int i = 0; i < threads_cnt; i++) {
            pthread_create(&threads[i], NULL, listen_routine, svr);
        }
    }

    listen_routine(svr);

    if (threads_cnt > 0) {
        for (int i = 0; i < threads_cnt; i++) {
            pthread_join(threads[i], NULL);
        }
        free(threads);
    }
}

struct server_info *create_server(const char *addr, uint16_t port)
{
    int64_t nfds;
    struct server_info *svr;
    size_t svr_len;

    if (UNLIKELY((nfds = get_nofile_limit()) < 0)) {
        LOG_ERROR("Failed to get max fd count\n");
        goto EXIT;
    }

    svr_len =
        sizeof(struct server_info) + sizeof(struct server_connection) * nfds;
    if (UNLIKELY((svr = malloc(svr_len)) == NULL)) {
        LOG_ERROR("Failed to allocate\n");
        goto EXIT;
    }
    memset(svr, 0, svr_len);

    if (UNLIKELY((svr->listen_fd = create_listening_socket(addr, port)) < 0)) {
        LOG_ERROR("Failed to create listening socket\n");
        goto FREE;
    }

    return svr;
// fall through
FREE:
    free(svr);
EXIT:
    return NULL;
}
