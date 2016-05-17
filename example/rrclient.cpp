#include <stdio.h>
#include <pthread.h>
#include "zmq.hpp"
#include "canna_core.h"

int main(int argc, char** argv)
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_ROUTER);
    
    socket.setsockopt(ZMQ_IDENTITY, "APPPLE", 6);
    socket.connect("tcp://localhost:5555");

    canna_sleep(3000);

    while (true) {
        printf("=================================\n");
        canna_sendmore(socket, "PURPLE");
        canna_send(socket, "Hello World!!!");

        std::string identity = canna_recv(socket);
        std::string reply = canna_recv(socket);
        printf("IDENTITY: %s\n", identity.c_str());
        printf("REPLY: %s\n", reply.c_str());
        printf("=================================\n");
        canna_sleep(3000);
    }

    return 0;
}
