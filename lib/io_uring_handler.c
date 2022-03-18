#include <liburing.h>
#include <sys/types.h>
#include <unistd.h>

#include "server.h"

#define QD (1024)

#define IO_URING_OP_ACCEPT (1)
#define IO_URING_OP_READ (2)
#define IO_URING_OP_WRITE (3)

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

static void io_uring_co_echo_handler(coroutine *co, void *data) {}

void io_uring_listen_loop(pthread_t tid, struct server_info *svr)
{
    struct io_uring ring;
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

    struct server_connection *listen_conn = &svr->conns[svr->listen_fd];
    io_uring_push_accept(svr->listen_fd, listen_conn, &ring);

    while (1) {
        io_uring_submit_and_wait(&ring, 1);
        int ret;
        unsigned head;
        unsigned count = 0;
        io_uring_for_each_cqe(&ring, head, cqe)
        {
            count++;
            ret = cqe->res;
            struct server_connection *conn =
                (struct server_connection *)cqe->user_data;
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
                } else {
                    LOG_INFO("socket accepted...\n");
                }

                struct server_connection *acc_conn = &svr->conns[ret];
                if (UNLIKELY(__prepare_connection(tid, ret, acc_conn,
                                                  io_uring_co_echo_handler) <
                             0)) {
                    LOG_ERROR("Failed to create connection...\n");
                    __close(ret);
                    goto ACCEPT_AGAIN;
                }

                // push new fd to wait for read
                io_uring_push_read(ret, acc_conn->buf.buf,
                                   acc_conn->buf.capacity, (void *)acc_conn,
                                   &ring);
            ACCEPT_AGAIN:
                io_uring_push_accept(svr->listen_fd, listen_conn, &ring);
            } else {
                /* co_resume(conn->coro); */
            }
        }
        /* io_uring_cqe_seen(&ring, cqe); */
        io_uring_cq_advance(&ring, count);
    }
}
