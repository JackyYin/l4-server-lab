#include <liburing.h>
#include <sys/types.h>
#include <unistd.h>

#include "http_parser.h"
#include "iohandler.h"

#define QD (1024)

#define IO_URING_OP_ACCEPT (1)
#define IO_URING_OP_READ (2)
#define IO_URING_OP_WRITE (3)

static struct io_uring ring;

static void io_uring_push_accept(int fd, struct server_connection *conn,
                                 struct io_uring *ring)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_accept(sqe, fd, NULL, NULL, 0);

    conn->action = IO_URING_OP_ACCEPT;
    sqe->user_data = (uint64_t)conn;
}

static void io_uring_push_read(int fd, void *buf, size_t buflen,
                               struct server_connection *conn,
                               struct io_uring *ring)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_recv(sqe, fd, buf, buflen, 0);

    conn->action = IO_URING_OP_READ;
    sqe->user_data = (uint64_t)conn;
}

static void io_uring_push_write(int fd, void *buf, size_t buflen,
                                struct server_connection *conn,
                                struct io_uring *ring)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_send(sqe, fd, buf, buflen, 0);

    conn->action = IO_URING_OP_WRITE;
    sqe->user_data = (uint64_t)conn;
}

__attribute__((unused)) static void io_uring_co_echo_handler(coroutine *co,
                                                             void *data)
{
    struct server_connection *conn = (struct server_connection *)data;

    while (1) {
        int to_yield = 0;
        int ret = co->yielded;

        switch (conn->action) {
        case IO_URING_OP_READ: {
            if (UNLIKELY(ret < 0)) {
                LOG_ERROR("read error: %d...\n", ret);
                goto YIELD;
            }

            if (UNLIKELY(ret == 0)) {
                LOG_INFO("remote peer closed\n");
                goto RESET;
            }

            to_yield = IO_URING_OP_WRITE;
            goto YIELD;
        }
        case IO_URING_OP_WRITE: {
            // this mean we've write all data
            if (ret < conn->buf.capacity) {
                goto RESET;
            }

            to_yield = IO_URING_OP_READ;
            goto YIELD;
        }
        }
    RESET:
        __close(conn->fd);
        memset(conn->buf.buf, 0, conn->buf.capacity);
        conn->buf.len = 0;

    YIELD:
        co_yield(co, to_yield);
    }
}

static void io_uring_co_http_handler(coroutine *co, void *data)
{
    struct server_connection *conn = (struct server_connection *)data;

    char fresbuf[2048] = {0};
    struct http_request req;
    struct http_response res;
    if (UNLIKELY((req.headers = kv_init()) == NULL)) {
        LOG_ERROR("failed to init kv struct...\n");
        return;
    }
    if (UNLIKELY((req.query = kv_init()) == NULL)) {
        LOG_ERROR("failed to init kv struct...\n");
        free(req.headers);
        return;
    }
    if (UNLIKELY((res.headers = kv_init()) == NULL)) {
        LOG_ERROR("failed to init kv struct...\n");
        free(req.headers);
        free(req.query);
        return;
    }

    while (1) {
        int to_yield = 0;
        int yielded = co->yielded;

        switch (conn->action) {
        case IO_URING_OP_READ: {
            if (UNLIKELY(yielded < 0)) {
                LOG_ERROR("read error: %d...\n", yielded);
                goto YIELD;
            }

            if (UNLIKELY(yielded == 0)) {
                LOG_INFO("remote peer closed\n");
                goto RESET;
            }

            // request entity too large
            if (UNLIKELY(yielded == conn->buf.capacity)) {
                char res[] = RESPONSE_413;
                io_uring_push_write(conn->fd, res, strlen(res), (void *)conn,
                                    &ring);
                goto YIELD;
            }

            if (!http_parse_request(&req, conn->buf.buf, yielded)) {
                char res[] = RESPONSE_400;
                io_uring_push_write(conn->fd, res, strlen(res), (void *)conn,
                                    &ring);
                goto YIELD;
            }

            route_handler *rh = NULL;
            if ((rh = find_router(req.path, req.method)) == NULL) {
                char res[] = RESPONSE_404;
                io_uring_push_write(conn->fd, res, strlen(res), (void *)conn,
                                    &ring);
                goto YIELD;
            }

            rh(&req, &res);
            if (http_compose_response(&req, &res, fresbuf)) {
                io_uring_push_write(conn->fd, fresbuf, strlen(fresbuf),
                                    (void *)conn, &ring);
            } else {
                char res[] = RESPONSE_400;
                io_uring_push_write(conn->fd, res, strlen(res), (void *)conn,
                                    &ring);
            }

            goto YIELD;
        }
        case IO_URING_OP_WRITE: {
            // this mean we've write all data
            if (yielded < conn->buf.capacity) {
                goto RESET;
            }

            to_yield = IO_URING_OP_READ;
            goto YIELD;
        }
        }
    RESET:
        __close(conn->fd);
        memset(conn->buf.buf, 0, conn->buf.capacity);
        conn->buf.len = 0;
        memset(&req, 0, sizeof(struct http_request) - sizeof(struct kv *) * 2);
        kv_reset(req.headers);
        kv_reset(req.query);
        res.status = 0;
        kv_reset(res.headers);
        memset(fresbuf, 0, 2048);

    YIELD:
        co_yield(co, to_yield);
    }
}

void io_uring_listen_loop(pthread_t tid, struct server_info *svr)
{
    struct io_uring_params ring_params;
    struct io_uring_cqe *cqe;
    memset(&ring_params, 0, sizeof(struct io_uring_params));

    int tmp = io_uring_queue_init_params(QD, &ring, &ring_params);
    if (UNLIKELY(tmp < 0)) {
        LOG_ERROR("[%lu] Failed to init io_uring: %s, %d\n", tid,
                  strerror(errno), tmp);
        return;
    }
    if (UNLIKELY(!(ring_params.features & IORING_FEAT_FAST_POLL))) {
        LOG_ERROR("IORING_FAST_POLL is not available");
        return;
    }

    struct server_connection *conn = &svr->conns[svr->listen_fd];
    io_uring_push_accept(svr->listen_fd, conn, &ring);
    while (1) {
        io_uring_submit_and_wait(&ring, 1);
        int ret;
        unsigned head;
        unsigned count = 0;
        io_uring_for_each_cqe(&ring, head, cqe)
        {
            count++;
            ret = cqe->res;
            conn = (struct server_connection *)cqe->user_data;
            if (UNLIKELY(!conn)) {
                LOG_ERROR("Something went wrong, no user_data in cqe!\n");
                continue;
            }

            if (conn->action == IO_URING_OP_ACCEPT) {
                // check accept4 return
                if (UNLIKELY(ret < 0)) {
                    if (ret == -EAGAIN)
                        goto ACCEPT_AGAIN;

                    LOG_ERROR("[%lu] Something went wrong with accept: %d\n",
                              tid, ret);
                    exit(1);
                }

                conn = &svr->conns[ret];
                if (UNLIKELY(!conn->coro &&
                             __prepare_connection(tid, ret, conn,
                                                  io_uring_co_http_handler) <
                                 0)) {
                    LOG_ERROR("Failed to create connection...\n");
                    __close(ret);
                    goto ACCEPT_AGAIN;
                }

                // push new fd to wait for read
                io_uring_push_read(ret, conn->buf.buf, conn->buf.capacity,
                                   (void *)conn, &ring);
            ACCEPT_AGAIN:
                io_uring_push_accept(svr->listen_fd,
                                     &svr->conns[svr->listen_fd], &ring);
                continue;
            }

            if (UNLIKELY(!conn->coro)) {
                LOG_ERROR("Something went wrong, coroutine not found!\n");
                continue;
            }

            co_resume_value(conn->coro, ret);
        }
        /* io_uring_cqe_seen(&ring, cqe); */
        io_uring_cq_advance(&ring, count);
    }
}
