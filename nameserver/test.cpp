#include <string>
#include <iostream>

#include "zmq.hpp"
#include "canna_core.h"

int main(int argc, char ** argv)
{
    zmq::context_t context(1);
    zmq::socket_t requester(context, ZMQ_REQ);
    requester.connect("tcp://localhost:5555");
    zmq::socket_t subscriber(context, ZMQ_SUB);
    subscriber.connect("tcp://localhost:5556");
    
    const char * filter = "";
    subscriber.setsockopt(ZMQ_SUBSCRIBE, filter, strlen(filter));

    canna_sleep(2000);

    canna_send(requester, "cluster");
    
    zmq::pollitem_t pollset[] = {
        {(void *)requester, 0, ZMQ_POLLIN, 0},
        {(void *)subscriber, 0, ZMQ_POLLIN, 0}
    };

    while (true) {
        zmq::poll(pollset, 2, 0);

        if (pollset[0].revents & ZMQ_POLLIN) {
            std::string res = canna_recv(requester);
            std::cout << "RES: " << res << std::endl;
        }

        if (pollset[1].revents & ZMQ_POLLIN) {
            std::string sub = canna_recv(subscriber);
            std::cout << "SUB: " << sub << std::endl;
        }
    }

    return 0;
}
