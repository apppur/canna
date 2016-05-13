#include <stdio.h>
#include "zmq.hpp"
#include "canna_core.h"

int main(int argc, char** argv)
{
    zmq::context_t context(1);
    
    zmq::socket_t sink(context, ZMQ_ROUTER);
    sink.setsockopt(ZMQ_IDENTITY, "PURPLE", 6);
    sink.bind("inproc://identity.inproc");

    zmq::socket_t anonymous(context, ZMQ_REQ);
    anonymous.connect("inproc://identity.inproc");

    canna_send(anonymous, "ROUTER uses a generated 5 byte indentity");
    canna_dump(sink);

    zmq::socket_t identified(context, ZMQ_REQ);
    identified.setsockopt(ZMQ_IDENTITY, "APPPUR", 6);
    identified.connect("inproc://identity.inproc");

    canna_send(identified, "ROUTER socket use REQ's identity");
    canna_dump(sink);

    zmq::socket_t apple(context, ZMQ_ROUTER);
    apple.setsockopt(ZMQ_IDENTITY, "APPPLE", 6);
    apple.connect("inproc://identity.inproc");

    canna_sendmore(apple, "PURPLE");
    canna_sendmore(apple, "");
    canna_send(apple, "ROUTER socket use REQ's identity");
    canna_dump(sink);
    printf("*******************************************\n");

    return 0;
}
