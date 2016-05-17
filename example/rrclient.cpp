#include <stdio.h>
#include <pthread.h>
#include "zmq.hpp"
#include "canna_core.h"

int main(int argc, char** argv)
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_ROUTER);
    
    socket.setsockopt(ZMQ_IDENTITY, "APPPLE", 6);
    //socket.connect("tcp://localhost:5555");
    socket.connect("ipc://purple.ipc");

    zmq::socket_t back(context, ZMQ_ROUTER);
    back.setsockopt(ZMQ_IDENTITY, "APPPLE1", 7);
    back.bind("ipc://apple.ipc");

    canna_sleep(3000);

    zmq::pollitem_t pollset[] = {
        {(void *)socket, 0, ZMQ_POLLIN, 0},
        {(void *)back, 0, ZMQ_POLLIN, 0}
    };

    while (true) {
        printf("=================================\n");
        canna_sendmore(socket, "PURPLE");
        canna_send(socket, "Hello World!!!");

        zmq::poll(pollset, 2, 0);
        if (pollset[0].revents & ZMQ_POLLIN) {

            std::string identity = canna_recv(socket);
            std::string reply = canna_recv(socket);
            printf("IDENTITY: %s\n", identity.c_str());
            printf("REPLY: %s\n", reply.c_str());
            printf("=================================\n");
        }

        if (pollset[1].revents & ZMQ_POLLIN) {
            printf("=================================\n");
            std::string id1 = canna_recv(back);
            std::string con = canna_recv(back);
            printf("ID: %s, Contant: %s\n", id1.c_str(), con.c_str());
            printf("=================================\n");
        }
        canna_sleep(3000);
    }

    return 0;
}
