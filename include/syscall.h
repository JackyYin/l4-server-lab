#ifndef _SYSCALL_H
#define _SYSCALL_H

#if defined(__x86_64__)

#include "arch/x86/syscall.h"
#include <linux/io_uring.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/syscall.h>

/*
 * Reference of syscall number:
 * https://github.com/torvalds/linux/blob/master/arch/x86/entry/syscalls/syscall_64.tbl
 */

#ifndef __NR_read
#define __NR_read 0
#endif

#ifndef __NR_write
#define __NR_write 1
#endif

#ifndef __NR_close
#define __NR_close 3
#endif

#ifndef __NR_socket
#define __NR_socket 41
#endif

#ifndef __NR_accept
#define __NR_accept 43
#endif

#ifndef __NR_bind
#define __NR_bind 49
#endif

#ifndef __NR_listen
#define __NR_listen 50
#endif

#ifndef __NR_getrlimit
#define __NR_getrlimit 97
#endif

#ifndef __NR_epoll_create
#define __NR_epoll_create 213
#endif

#ifndef __NR_epoll_wait
#define __NR_epoll_wait 232
#endif

#ifndef __NR_epoll_ctl
#define __NR_epoll_ctl 233
#endif

#ifndef __NR_epoll_pwait
#define __NR_epoll_pwait 281
#endif

#ifndef __NR_accept4
#define __NR_accept4 288
#endif

#ifndef __NR_epoll_create1
#define __NR_epoll_create1 291
#endif

#ifndef __NR_io_uring_setup
#define __NR_io_uring_setup 425
#endif

#ifndef __NR_io_uring_enter
#define __NR_io_uring_enter 426
#endif

#ifndef __NR_io_uring_register
#define __NR_io_uring_register 427
#endif

static inline int __read(int fd, void *buf, size_t count)
{
    return (int)syscall_3(__NR_read, fd, buf, count);
}

static inline int __write(int fd, const void *buf, size_t count)
{
    return (int)syscall_3(__NR_write, fd, buf, count);
}

static inline int __close(int fd) { return (int)syscall_1(__NR_close, fd); }

static inline int __socket(int domain, int type, int protocol)
{
    return (int)syscall_3(__NR_socket, domain, type, protocol);
}

static inline int __bind(int sockfd, const struct sockaddr *addr,
                         socklen_t addrlen)
{
    return (int)syscall_3(__NR_bind, sockfd, addr, addrlen);
}

static inline int __listen(int sockfd, int backlog)
{
    return (int)syscall_2(__NR_listen, sockfd, backlog);
}

static inline int __accept4(int socket, struct sockaddr *restrict addr,
                            socklen_t *restrict addrlen, int flags)
{
    return (int)syscall_4(__NR_accept4, socket, addr, addrlen, flags);
}

static inline int __getrlimit(int resource, struct rlimit *rlim)
{
    return (int)syscall_2(__NR_getrlimit, resource, rlim);
}

static inline int __epoll_create1(int flags)
{
    return (int)syscall_1(__NR_epoll_create1, flags);
}

static inline int __epoll_ctl(int epfd, int op, int fd,
                              struct epoll_event *event)
{
    return (int)syscall_4(__NR_epoll_ctl, epfd, op, fd, event);
}

static inline int __epoll_wait(int epfd, struct epoll_event *events,
                               int maxevents, int timeout)
{
    return (int)syscall_4(__NR_epoll_wait, epfd, events, maxevents, timeout);
}

static inline int __io_uring_setup(unsigned int entries,
                                   struct io_uring_params *p)
{
    return (int)syscall_2(__NR_io_uring_setup, entries, p);
}

static inline int __io_uring_enter(unsigned int fd, unsigned int to_submit,
                                   unsigned int min_complete,
                                   unsigned int flags, sigset_t *sig)
{
    return (int)syscall_5(__NR_io_uring_enter, fd, to_submit, min_complete,
                          flags, sig);
}

static inline int __io_uring_register(unsigned int fd, unsigned int opcode,
                                      void *arg, unsigned int nr_args)
{
    return (int)syscall_4(__NR_io_uring_register, fd, opcode, arg, nr_args);
}

#else
#error Only x86_64 supported.
#endif

#endif
