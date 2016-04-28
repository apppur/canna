
#include <string>
#include <iostream>
#include <stdio.h>

#include "zmq.hpp"

int main(int argc, char** argv)
{
    char buffer[256];
    int major, minor, patch;
    zmq::version(&major, &minor, &patch);
    printf("current zeromq version is %d.%d.%d\n", major, minor, patch);

    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REQ);
    std::cout << "connecting to dbserver ..." << std::endl;
    socket.connect("tcp://localhost:5555");

    {
        zmq::message_t request(24);
        memcpy(request.data(), "SET nickname applepurple", 24);
        socket.send(request);

        zmq::message_t reply;
        socket.recv(&reply);
        memset(buffer, 0, 256);
        memcpy(buffer, static_cast<char*>(reply.data()), reply.size());

        printf("response: %s\n", buffer);
    }

    {
        zmq::message_t request(12);
        memcpy(request.data(), "GET nickname", 12);
        socket.send(request);

        zmq::message_t reply;
        socket.recv(&reply);
        memset(buffer, 0, 256);
        memcpy(buffer, static_cast<char*>(reply.data()), reply.size());

        printf("response: %s\n", buffer);
    }

    return 0;
}
