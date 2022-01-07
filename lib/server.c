#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/types.h>
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

static int epoll_modify(int epfd, int op, int fd, uint32_t events, void *data)
{
    struct epoll_event eevt = {.events = events | EPOLLRDHUP,
                               .data = {.ptr = data}};

    return epoll_ctl(epfd, op, fd, &eevt);
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

    if (UNLIKELY((epoll_modify(svr->epoll_fd, EPOLL_CTL_ADD, svr->listen_fd,
                               EPOLLIN | EPOLLET, NULL)) < 0)) {
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

static void co_handle_request(coroutine *co, void *data)
{
    struct server_connection *conn = (struct server_connection *)data;

    while (1) {
        if (conn->action & EPOLLIN) {
            bool stop = false;
            ssize_t cnt = 0;
            size_t want_to_read = 0;

            while (1) {
                want_to_read = conn->capacity - conn->len;

                if (UNLIKELY(want_to_read == 0)) {
                    char *newbuf = NULL;
                    if (UNLIKELY(
                            (newbuf = realloc(conn->buf, conn->capacity * 2)) !=
                            NULL)) {
                        LOG_ERROR("Failed to realloc bigger buf!\n");
                        stop = true;
                        break;
                    }
                    conn->buf = newbuf;
                    conn->capacity *= 2;
                    want_to_read = conn->capacity - conn->len;
                }

                cnt = read(conn->fd, (void *)(conn->buf + conn->len),
                           want_to_read);

                if (UNLIKELY(cnt < 0)) {
                    // EAGAIN or EWOULDBLOCK means that we have read all data
                    if (UNLIKELY(errno != EAGAIN && errno != EWOULDBLOCK)) {
                        LOG_ERROR("Something went wrong with read: %s\n",
                                  strerror(errno));
                        stop = true;
                    }
                    break;
                }

                // The remote peer close the connection
                if (UNLIKELY(cnt == 0)) {
                    stop = true;
                    break;
                }

                conn->len += cnt;
            }

            if (stop) {
                // Now the coroutine status becomes CO_STATUS_STOPPED
                return;
            }

            co_yield(co, EPOLLOUT);
        } else if (conn->action & EPOLLOUT) {
            bool stop = false;
            ssize_t cnt = 0;
            size_t accu = 0;
            size_t want_to_write = 0;

            while (1) {
                want_to_write = conn->len - accu;

                // Write finished
                if (want_to_write == 0) {
                    break;
                }

                cnt =
                    write(conn->fd, (void *)(conn->buf + accu), want_to_write);

                if (UNLIKELY(cnt < 0)) {
                    // EAGAIN or EWOULDBLOCK means that we have write all data
                    if (UNLIKELY(errno != EAGAIN && errno != EWOULDBLOCK)) {
                        LOG_ERROR("Something went wrong with write: %s\n",
                                  strerror(errno));
                        stop = true;
                    }
                    break;
                }

                // The remote peer close the connection
                if (UNLIKELY(cnt == 0)) {
                    stop = true;
                    break;
                }

                accu += cnt;
            }

            if (stop) {
                // Now the coroutine status becomes CO_STATUS_STOPPED
                return;
            }

            // Now restore the buffer len index
            conn->len = 0;

            co_yield(co, EPOLLIN);
        }
    }
}

static bool accept_connection(struct server_info *svr)
{
    struct sockaddr_in so_addr;
    socklen_t so_addr_len = sizeof(struct sockaddr_in);
    int64_t accept_fd;

    while (1) {
        accept_fd =
            accept4(svr->listen_fd, (struct sockaddr *)&so_addr,
                    (socklen_t *)&so_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);

        if (UNLIKELY(accept_fd < 0)) {
            switch (errno) {
            // Retry
            case ENETDOWN:
            case EPROTO:
            case ENOPROTOOPT:
            case EHOSTDOWN:
            case ENONET:
            case EHOSTUNREACH:
            case EOPNOTSUPP:
            case ENETUNREACH: {
                LOG_ERROR("Something went wrong with accept4: %s\n",
                          strerror(errno));
                continue;
            }
            // No more socket waiting for acception
            case EAGAIN: // EWOULDBLOCK
                goto EXIT;
            // Something went wrong
            default: {
                LOG_ERROR("Something went wrong with accept4: %s\n",
                          strerror(errno));
                goto ERR_EXIT;
            }
            }
        }

        if (UNLIKELY((epoll_modify(svr->epoll_fd, EPOLL_CTL_ADD, accept_fd,
                                   EPOLLIN, (void *)&svr->conns[accept_fd])) <
                     0)) {
            close(accept_fd);
            continue;
        }

        if (UNLIKELY((svr->conns[accept_fd].coro =
                          co_new((co_func)co_handle_request,
                                 (void *)&svr->conns[accept_fd])) == NULL)) {
            close(accept_fd);
            continue;
        }

        if (UNLIKELY((svr->conns[accept_fd].buf = malloc(1024)) == NULL)) {
            svr->conns[accept_fd].capacity = 0;
        } else {
            svr->conns[accept_fd].capacity = 1024;
        }
        svr->conns[accept_fd].fd = accept_fd;
        svr->conns[accept_fd].action = 0;
    }

EXIT:
    return true;

ERR_EXIT:
    return false;
}

void start_listening(struct server_info *svr)
{
    int64_t nfds;
    uint16_t max_events = 100; // TODO: not sure what number is proper
    struct epoll_event *pevts = NULL;
    struct epoll_event *pevt = NULL;
    struct server_connection *conn = NULL;

    if (UNLIKELY((pevts = malloc(sizeof(struct epoll_event) * max_events)) ==
                 NULL)) {
        LOG_ERROR("Failed to allocate\n");
        goto EXIT;
    }

    while (1) {
        nfds = epoll_wait(svr->epoll_fd, pevts, max_events, -1);

        if (UNLIKELY(nfds < 0)) {
            LOG_ERROR("Something goes wrong with epoll_wait: %s\n",
                      strerror(errno));
            goto FREE;
        }

        // now, foreach ready fd
        for (int i = 0; i < nfds; i++) {
            pevt = pevts + i;
            // This should be the listen fd
            if (pevt[i].data.ptr == NULL) {
                // Now we ignore error
                accept_connection(svr);
                continue;
            }

            conn = pevt[i].data.ptr;
            if (UNLIKELY(pevt[i].events & EPOLLERR)) {
                close(conn->fd);
                continue;
            }

            // Treating hang-up like error ?
            if (UNLIKELY((pevt[i].events & EPOLLHUP) ||
                         (pevt[i].events & EPOLLRDHUP))) {
                close(conn->fd);
                continue;
            }

            if (UNLIKELY(!conn->coro)) {
                LOG_ERROR("connection CORO not initialized...\n");
                continue;
            }

            conn->action = pevt[i].events;

            // Switch read/write interest list
            epoll_modify(svr->epoll_fd, EPOLL_CTL_MOD, conn->fd,
                         co_resume(conn->coro), (void *)conn);

            if (conn->coro->status == CO_STATUS_STOPPED) {
                close(conn->fd);
                co_free(conn->coro);
                conn->coro = NULL;
                conn->action = 0;
            }
        }
    }

// fall through
FREE:
    free(pevts);

EXIT:
}
