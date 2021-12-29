#include <stdio.h>
#include <stdlib.h>

#include "socket.h"

int main()
{
    char addr[] = "127.0.0.1";
    int port = 8080;
    int fd;

    fd = create_listening_socket((const char *)addr, (uint16_t)port);

    if (fd > 0) {
        printf("socket successfully created: %d\n", fd);
    }
}
