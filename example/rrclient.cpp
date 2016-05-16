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

    canna_sleep(5000);

    canna_sendmore(socket, "PURPLE");
    canna_sendmore(socket, "");
    canna_send(socket, "Hello World!!!");

    while (true) {
        canna_sleep(5000);
    }

    return 0;
}
