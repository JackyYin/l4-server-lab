#ifndef _IOURING_UTILS_H
#define _IOURING_UTILS_H

#include <liburing.h>

#include "server.h"

#define IO_URING_OP_ACCEPT (1)
#define IO_URING_OP_RECV (2)
#define IO_URING_OP_SEND (3)
#define IO_URING_OP_SPLICE (4)

void io_uring_push_accept(struct io_uring *ring, int fd,
                          struct server_connection *conn, uint8_t flags);

void io_uring_push_read(struct io_uring *ring, int fd, void *buf, size_t buflen,
                        uint8_t flags);

void io_uring_push_recv(struct io_uring *ring, int fd, void *buf, size_t buflen,
                        struct server_connection *conn, uint8_t flags);

void io_uring_push_send(struct io_uring *ring, int fd, void *buf, size_t buflen,
                        struct server_connection *conn, int send_flags,
                        uint8_t flags);

void io_uring_push_splice(struct io_uring *ring, int fd_in, int64_t off_in,
                          int fd_out, int64_t off_out, unsigned int len,
                          struct server_connection *conn, uint8_t flags);
#endif
