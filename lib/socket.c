#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "socket.h"
#include "syscall.h"

static uint64_t get_somaxconn()
{
    uint64_t somaxconn = 0;
    FILE *file = fopen("/proc/sys/net/core/somaxconn", "r");

    if (LIKELY(file)) {
        /* fseek(file, 0, SEEK_END); */
        /* fsize = ftell(); */
        /* fseek(file, 0, SEEK_SET); */
        if (UNLIKELY(fscanf(file, "%lu", &somaxconn) == 0)) {
            LOG_ERROR("Failed to read somaxconn value\n");
        }
    }

    return somaxconn;
}

int create_listening_socket(const char *addr, uint16_t port)
{
    int fd;
    int backlog;

    if (UNLIKELY(
            (fd = __socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                           0)) < 0)) {
        LOG_ERROR("Failed to open socket: %s\n", strerror(errno));
        goto EXIT;
    }

    struct sockaddr_in so_addr = {.sin_family = AF_INET,
                                  .sin_port = htons(port),
                                  .sin_zero = {[0 ... 7] = 0}};
    if (UNLIKELY(!inet_aton(addr, &so_addr.sin_addr))) {
        LOG_ERROR("Failed to parse listen address\n");
        goto CLEANUP;
    }

    if (UNLIKELY(__bind(fd, (struct sockaddr *)&so_addr, sizeof(so_addr)) ==
                 -1)) {
        LOG_ERROR("Failed to bind: %s\n", strerror(errno));
        goto CLEANUP;
    }

    if (UNLIKELY((backlog = get_somaxconn()) == 0)) {
        LOG_ERROR("Failed to get socket max connection\n");
        goto CLEANUP;
    }

    if (UNLIKELY(__listen(fd, backlog) == -1)) {
        LOG_ERROR("Failed to listen: %s\n", strerror(errno));
        goto CLEANUP;
    }

    return fd;
// fall through
CLEANUP:
    __close(fd);

EXIT:
    return -1;
}
