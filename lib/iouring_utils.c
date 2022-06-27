#define _GNU_SOURCE
#include <fcntl.h>

#include "iouring_utils.h"

void io_uring_push_accept(struct io_uring *ring, int fd,
                          struct server_connection *conn, uint8_t flags)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_accept(sqe, fd, NULL, NULL, 0);

    conn->action = IO_URING_OP_ACCEPT;
    sqe->user_data = (uint64_t)conn;
    sqe->flags = flags;
}

void io_uring_push_read(struct io_uring *ring, int fd, void *buf, size_t buflen,
                        uint8_t flags)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_read(sqe, fd, buf, buflen, 0);

    sqe->user_data = 0;
    sqe->flags = flags;
}

void io_uring_push_recv(struct io_uring *ring, int fd, void *buf, size_t buflen,
                        struct server_connection *conn, uint8_t flags)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_recv(sqe, fd, buf, buflen, 0);

    conn->action = IO_URING_OP_RECV;
    sqe->user_data = (uint64_t)conn;
    sqe->flags = flags;
}

void io_uring_push_send(struct io_uring *ring, int fd, void *buf, size_t buflen,
                        struct server_connection *conn, int send_flags,
                        uint8_t flags)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_send(sqe, fd, buf, buflen, send_flags);

    conn->action = IO_URING_OP_SEND;
    conn->total_steps++;
    sqe->user_data = (uint64_t)conn;
    sqe->flags = flags;
}

void io_uring_push_splice(struct io_uring *ring, int fd_in, int64_t off_in,
                          int fd_out, int64_t off_out, unsigned int len,
                          struct server_connection *conn, uint8_t flags)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_splice(sqe, fd_in, off_in, fd_out, off_out, len,
                         SPLICE_F_NONBLOCK);

    conn->action = IO_URING_OP_SPLICE;
    conn->total_steps++;
    sqe->user_data = (uint64_t)conn;
    sqe->flags = flags;
}
