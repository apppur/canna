
#include <pthread.h>
#include <iostream>

#include "canna_core.h"

static void* work_thread(void *arg)
{
    zmq::context_t context(1);
    zmq::socket_t worker(context, ZMQ_DEALER);

    canna_setid(worker);
    worker.connect("tcp:://localhost:5671");

    int total = 0;

    while (true) {
        canna_sendmore(worker, "");
        canna_send(worker, "READY");

        canna_recv(worker);
        std::string workload = canna_recv(worker);
        if ("FIRED!" == workload) {
            std::cout << "PROCESSED: " << total << " TASKS" << std::endl;
            break;
        }
        total++;

        canna_sleep(CANNA_RAND(500)+1.0);
    }

    return nullptr;
}

int main(int argc, char** argv)
{
    zmq::context_t context(1);
    zmq::socket_t broker(context, ZMQ_ROUTER);
    broker.bind("tcp:://*:5671");

    const int work_count = 10;
    pthread_t workers[work_count];
    for (int i = 0; i < work_count; i++) {
        pthread_create(workers+i, nullptr, work_thread, (void *)(intptr_t)i);
    }

    int64_t end_time = canna_gettime() + 5000;
    int worker_fired = 0;
    while (true) {
        std::string identity = canna_recv(broker);
        {
            canna_recv(broker);
            canna_recv(broker);
        }

        canna_sendmore(broker, identity);
        canna_sendmore(broker, "");

        if (canna_gettime() < end_time) {
            canna_send(broker, "WORK HARDER");
        } else {
            canna_send(broker, "FIRED!");
            if (++worker_fired == work_count) {
                break;
            }
        }
    }

    for (int i = 0; i < work_count; i++) {
        pthread_join(workers[i], nullptr);
    }

    return 0;
}
