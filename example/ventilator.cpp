#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

#include "zmq.hpp"

#include "canna_core.h"
#include "canna_random.h"

int main(int argc, char** argv)
{
    zmq::context_t context(1);

    zmq::socket_t sender(context, ZMQ_PUSH);
    sender.bind("tcp://*:5557");

    std::cout << "Press Enter when the workers are ready: " << std::endl;
    getchar ();
    std::cout << "Sending tasks to workers…\n" << std::endl;

    zmq::socket_t sink(context, ZMQ_PUSH);
    sink.connect("tcp://localhost:5558");
    zmq::message_t message(2);
    memcpy(message.data(), "0", 1);
    sink.send(message);

    CaRandom random;
    random.SetSeed((unsigned int)(canna_gettime()));

    int count;
    int total;
    for (count = 0; count < 100; count++) {
        int workload;
        workload = random.Random(100) + 1;
        total += workload;

        message.rebuild(10);
        memset(message.data(), '\0', 10);
        sprintf((char *)message.data(), "%d", workload);
        sender.send(message);
    }

    std::cout << "Total expected cost: " << total << " msec" << std::endl;
    sleep(1);

    return 0;
}
