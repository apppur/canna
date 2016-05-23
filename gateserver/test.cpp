#include <stdio.h>
#include "socket_server.h"

int main(int argc, char **argv)
{
    socket_server server;
    server.initialize();
    int l = server.server_listen(100, nullptr, 8888, 32);
    server.server_start(101, l);
    struct socket_message result;
    int more;
    for ( ; ; ) {
        int type = server.poll(&result, &more);
        switch (type) {
            case SOCKET_EXIT:
                return -1;
            case SOCKET_DATA:
                printf("message(%lu) [id = %d] size = %d\n", result.opaque, result.id, result.ud);
                break;
            case SOCKET_CLOSE:
                printf("close(%lu) [id = %d]\n", result.opaque, result.id);
                break;
            case SOCKET_OPEN:
                printf("open(%lu) [id = %d], %s\n", result.opaque, result.id, result.data);
                break;
            case SOCKET_ERROR:
                printf("error(%lu) [id = %d]\n", result.opaque, result.id);
                break;
            case SOCKET_ACCEPT:
                printf("accept(%lu) [id = %d]\n", result.opaque, result.id);
                break;
        }
    }

    return 0;
}
