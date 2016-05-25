#include <stdio.h>
#include <pthread.h>
#include "socket_server.h"

static void * client(void * ud)
{
    socket_server * s = (socket_server *) ud;
    if (s == nullptr) {
        printf("socket server ptr: null\n");
        return nullptr;
    }
    sleep(5);
    int id = s->server_connect(400, "127.0.0.1", 8888);
    sleep(5);
    const char * info = "hello world";
    char * data = (char *)malloc(strlen(info));
    memcpy(data, info, strlen(info));
    s->server_send(id, data, strlen(info));

    while (true) {
        sleep(5);
    }
}

int main(int argc, char **argv)
{
    socket_server server;
    server.initialize();
    int l = server.server_listen(100, nullptr, 8888, 32);
    server.server_start(101, l);

    pthread_t pid;
    pthread_create(&pid, nullptr, client, &server);
    //pthread_join(pid, nullptr);

    printf("==========server poll run=============\n");
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
