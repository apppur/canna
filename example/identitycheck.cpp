#include "zmq.hpp"
#include "canna_core.h"

int main(int argc, char** argv)
{
    zmq::context_t context(1);
    
    zmq::socket_t sink(context, ZMQ_ROUTER);
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

    return 0;
}
