#include "socket_server.h"

int main(int argc, char **argv)
{
    socket_server server;
    server.initialize();
    server.server_listen(100, "127.0.0.1", 8888, 32);
    struct socket_message message;
    int more;
    server.poll(&message, &more);

    return 0;
}
