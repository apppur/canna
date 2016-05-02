#include <string>
#include <sstream>
#include <iostream>
#include <unistd.h>

#include "zmq.hpp"

int main(int argc, char** argv)
{
    zmq::context_t context(1);

    zmq::socket_t receiver(context, ZMQ_PULL);
    receiver.connect("tcp://localhost:5557");

    zmq::socket_t sender(context, ZMQ_PUSH);
    sender.connect("tcp://localhost:5558");

    while (true) {
        zmq::message_t message;
        int workload;

        receiver.recv(&message);
        std::string smessage(static_cast<char*>(message.data()), message.size());
        std::istringstream iss(smessage);
        iss >> workload;

        usleep(workload * 1000);

        message.rebuild();
        sender.send(message);
        std::cout << "." << std::flush;
    }

    return 0;
}
