#include <netinet/in.h>
#include <pthread.h>
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
#ifndef NDEBUG
    LOG_INFO("nofile soft limit: %lu\n", rlim.rlim_cur);
    LOG_INFO("nofile hard limit: %lu\n", rlim.rlim_max);
#endif
    return rlim.rlim_cur;
}

static int epoll_modify(int epfd, int op, int fd, uint32_t events, void *data)
{
    struct epoll_event eevt = {.events = events | EPOLLHUP | EPOLLET,
                               .data = {.ptr = data}};

    return epoll_ctl(epfd, op, fd, &eevt);
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

static void co_echo_handler(coroutine *co, void *data)
{
    pthread_t tid = pthread_self();
    struct server_connection *conn = (struct server_connection *)data;

    while (1) {
        if (conn->action & EPOLLIN) {
            bool stop = false;
            ssize_t cnt = 0;
            ssize_t want_to_read = 0;

            while (1) {
                want_to_read = conn->buf.capacity - conn->buf.len;

                if (UNLIKELY(want_to_read <= 0)) {
                    char *newbuf = NULL;
                    if (UNLIKELY((newbuf = realloc(conn->buf.buf,
                                                   conn->buf.capacity * 2)) ==
                                 NULL)) {
                        LOG_ERROR("[%lu] Failed to realloc bigger buf!\n", tid);
                        stop = true;
                        break;
                    }
                    conn->buf.buf = newbuf;
                    conn->buf.capacity *= 2;
                    want_to_read = conn->buf.capacity - conn->buf.len;
                }

                cnt = read(conn->fd, (void *)(conn->buf.buf + conn->buf.len),
                           want_to_read);

                if (UNLIKELY(cnt < 0)) {
                    // EAGAIN or EWOULDBLOCK means that we have read all data
                    if (UNLIKELY(errno != EAGAIN && errno != EWOULDBLOCK)) {
                        LOG_ERROR("[%lu] Something went wrong with read: %s\n",
                                  tid, strerror(errno));
                        stop = true;
                    }
                    break;
                }

                // read EOF
                if (UNLIKELY(cnt == 0)) {
                    break;
                }

                conn->buf.len += cnt;
            }

            if (stop) {
                // Now the coroutine status becomes CO_STATUS_STOPPED
                return;
            }

            co_yield(co, EPOLLOUT);
        } else if (conn->action & EPOLLOUT) {
            ssize_t cnt = 0;
            size_t accu = 0;
            size_t want_to_write = 0;

            while (1) {
                want_to_write = conn->buf.len - accu;

                // Write finished
                if (want_to_write == 0) {
                    break;
                }

                cnt = write(conn->fd, (void *)(conn->buf.buf + accu),
                            want_to_write);

                if (UNLIKELY(cnt < 0)) {
                    // EAGAIN or EWOULDBLOCK means that we have write all data
                    if (UNLIKELY(errno != EAGAIN && errno != EWOULDBLOCK)) {
                        LOG_ERROR("[%lu] Something went wrong with write: %s\n",
                                  tid, strerror(errno));
                    }
                    break;
                }

                // The remote peer close the connection
                if (UNLIKELY(cnt == 0)) {
                    break;
                }

                accu += cnt;
            }

            conn->buf.len = 0;
            // Now the coroutine status becomes CO_STATUS_STOPPED
            return;
        }
    }
}

static bool accept_connection(int epfd, struct server_info *svr)
{
    pthread_t tid = pthread_self();
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
                LOG_ERROR("[%lu] Something went wrong with accept4: %s\n", tid,
                          strerror(errno));
                continue;
            }
            // No more socket waiting for acception
            case EAGAIN: // EWOULDBLOCK
                goto EXIT;
            // Something went wrong
            default: {
                LOG_ERROR("[%lu] Something went wrong with accept4: %s\n", tid,
                          strerror(errno));
                goto ERR_EXIT;
            }
            }
        }

        /*
         * We don't accept this fd, because other thread is processing now
         */
        uint32_t old_ref_cnt;
        if ((old_ref_cnt = atomic_load(&(svr->conns[accept_fd].refcnt))) != 0)
            continue;

        /*
         * Now we increase refcnt, but race condition is still possible, we have
         * to make sure only 1 thread at a time
         */
        if (!atomic_compare_exchange_weak(&(svr->conns[accept_fd].refcnt),
                                          &old_ref_cnt, 1)) {
            continue;
        }

#ifndef NDEBUG
        LOG_INFO("[%lu] fd accepted: %ld\n", tid, accept_fd);
#endif

        if (UNLIKELY((epoll_modify(epfd, EPOLL_CTL_ADD, accept_fd, EPOLLIN,
                                   (void *)&svr->conns[accept_fd])) < 0)) {
            close(accept_fd);
            continue;
        }

        if (UNLIKELY((svr->conns[accept_fd].coro =
                          co_new((co_func)co_echo_handler,
                                 (void *)&svr->conns[accept_fd])) == NULL)) {
            LOG_ERROR("[%lu] Failed to create coroutine for fd: %ld\n", tid,
                      accept_fd);
            close(accept_fd);
            continue;
        }
#ifndef NDEBUG
        LOG_INFO("[%lu] created coro %p for fd: %ld\n", tid,
                 svr->conns[accept_fd].coro, accept_fd);
#endif
        // This fd never accept connection before
        if (!svr->conns[accept_fd].buf.buf) {
            if (UNLIKELY((svr->conns[accept_fd].buf.buf =
                              malloc(DEFAULT_SVR_BUFLEN)) == NULL)) {
                svr->conns[accept_fd].buf.capacity = 0;
            } else {
                svr->conns[accept_fd].buf.capacity = DEFAULT_SVR_BUFLEN;
            }
        }
        svr->conns[accept_fd].fd = accept_fd;
        svr->conns[accept_fd].action = 0;
    }

EXIT:
    return true;

ERR_EXIT:
    return false;
}

static void *listen_routine(void *arg)
{
    struct server_info *svr = (struct server_info *)arg;
    pthread_t tid = pthread_self();
    int epfd;

    if (UNLIKELY((epfd = epoll_create1(EPOLL_CLOEXEC)) == -1)) {
        LOG_ERROR("[%lu] Failed to create epoll instance: %s\n", tid,
                  strerror(errno));
        goto EXIT;
    }
#ifndef NDEBUG
    LOG_INFO("[%lu] epfd: %d\n", tid, epfd);
#endif
    if (UNLIKELY((epoll_modify(epfd, EPOLL_CTL_ADD, svr->listen_fd,
                               EPOLLIN | EPOLLEXCLUSIVE, NULL)) < 0)) {
        LOG_ERROR("[%lu] Failed to monitor listen fd: %s\n", tid,
                  strerror(errno));
        close(epfd);
        goto EXIT;
    }
    int64_t nfds;
    uint16_t max_events = 100; // TODO: not sure what number is proper
    struct epoll_event *pevts = NULL;
    struct epoll_event *pevt = NULL;
    struct server_connection *conn = NULL;

    if (UNLIKELY((pevts = malloc(sizeof(struct epoll_event) * max_events)) ==
                 NULL)) {
        LOG_ERROR("[%lu] Failed to allocate epoll_event array\n", tid);
        goto EXIT;
    }

    while (1) {
        nfds = epoll_wait(epfd, pevts, max_events, -1);

        if (UNLIKELY(nfds < 0)) {
            LOG_ERROR("[%lu] Something goes wrong with epoll_wait: %s\n", tid,
                      strerror(errno));
            goto FREE;
        }

        // now, foreach ready fd
        for (int i = 0; i < nfds; i++) {
            pevt = pevts + i;
            conn = pevt->data.ptr;
            // This should be the listen fd
            if (conn == NULL) {
                // Now we ignore error
                accept_connection(epfd, svr);
                continue;
            }

            if (UNLIKELY(pevt->events & EPOLLERR)) {
                LOG_ERROR("[%lu] epoll error\n", tid);
                close(conn->fd);
                continue;
            }

            // Treating hang-up like error ?
            if (UNLIKELY((pevt->events & (EPOLLHUP | EPOLLRDHUP)))) {
                LOG_ERROR("[%lu] epoll hung up\n", tid);
                close(conn->fd);
                continue;
            }

            if (UNLIKELY(!conn->coro)) {
#ifndef NDEBUG
                LOG_ERROR(
                    "[%lu] connection coroutine not initialized, fd: %d\n", tid,
                    conn->fd);
                exit(1);
#endif
                continue;
            }

            conn->action = pevt->events;

            int64_t yielded = co_resume(conn->coro);

            if (UNLIKELY(conn->coro->status == CO_STATUS_STOPPED)) {
                close(conn->fd);
                co_free(conn->coro);
                conn->coro = NULL;
                conn->action = 0;

                /*
                 * Ref count should be 0 after subtraction
                 */
#ifndef NDEBUG
                uint32_t old_ref_cnt = atomic_fetch_sub(&(conn->refcnt), 1);
                if (UNLIKELY(old_ref_cnt != 1)) {
                    LOG_ERROR("[%lu] race condtion on fd: %d\n", tid, conn->fd);
                    exit(1);
                }
                LOG_INFO("[%lu] fd closed: %d\n", tid, conn->fd);
#else
                atomic_fetch_sub(&(conn->refcnt), 1);
#endif
            } else {
                // Switch read/write interest list
                epoll_modify(epfd, EPOLL_CTL_MOD, conn->fd, yielded,
                             (void *)conn);
            }
        }
    }

// fall through
FREE:
    close(epfd);
    free(pevts);

EXIT:
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
