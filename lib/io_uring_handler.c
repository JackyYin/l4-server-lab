#include <liburing.h>
#include <sys/types.h>
#include <unistd.h>

#include "server.h"

#define QD (1024)

#define IO_URING_OP_ACCEPT (1)
#define IO_URING_OP_READ (2)
#define IO_URING_OP_WRITE (3)

// this structure can't be larger than 64 bytes
struct io_uring_user_data {
    void *ptr; // point to connection
    int op;
};

static void io_uring_push_accept(int fd, struct io_uring *ring)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_accept(sqe, fd, NULL, NULL, 0);

    struct io_uring_user_data *ud =
        (struct io_uring_user_data *)&sqe->user_data;
    ud->op = IO_URING_OP_ACCEPT:
}

static void io_uring_push_read(int fd, void *buf, size_t buflen, void *conn,
                               struct io_uring *ring)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_recv(sqe, fd, buf, buflen, 0);

    struct io_uring_user_data *ud =
        (struct io_uring_user_data *)&sqe->user_data;
    ud->op = IO_URING_OP_READ;
    ud->ptr = conn;
}

static void io_uring_co_echo_handler(coroutine *co, void *data)
{
}

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
    io_uring_push_accept(svr->listen_fd, &ring);

    while (1) {
        io_uring_submit_and_wait(&ring, 1);
        int ret;
        unsigned head;
        unsigned count = 0;
        io_uring_for_each_cqe(&ring, head, cqe)
        {
            count++;
            ret = cqe->res;
            struct io_uring_user_data *ud =
                (struct io_uring_user_data *)&cqe->user_data;
            if (ud->op == IO_URING_OP_ACCEPT) {
                // check accept4 return
                if (UNLIKELY(ret < 0)) {
                    if (ret != -EAGAIN) {
                        LOG_ERROR(
                            "[%lu] Something went wrong with accept: %d\n", tid,
                            ret);
                        exit(1);
                    }
                } else {
                    LOG_INFO("socket accepted...\n");
                }

                struct server_connection *conn = &svr->conns[ret];
                if (UNLIKELY(__prepare_connection(tid, ret, conn,
                                                  io_uring_co_echo_handler) < 0)) {
                    __close(ret);
                    goto ACCEPT_AGAIN;
                }

                // push new fd to wait for read
                io_uring_push_read(ret, conn->buf.buf, conn->buf.capacity,
                                   (void *)conn, &ring);
            ACCEPT_AGAIN:
                io_uring_push_accept(svr->listen_fd, &ring);
            } else if (ud->ptr) {
                struct server_connection *conn =
                    (struct server_connection *)ud->ptr;
                int64_t yielded = co_resume(conn->coro);
            } else {
                LOG_ERROR("Something went wrong, no user_data in cqe!\n");
            }
        }
        /* io_uring_cqe_seen(&ring, cqe); */
        io_uring_cq_advance(&ring, count);
    }
}
