#include <pthread.h>
#include <queue>
#include <iostream>
#include <assert.h>

#include "canna_core.h"

static void * client_thread(void *arg)
{
    zmq::context_t context(1);
    zmq::socket_t client(context, ZMQ_REQ);

    canna_setid(client);
    client.connect("tcp://localhost:5672");

    canna_send(client, "HELLO");
    std::string reply = canna_recv(client);
    std::cout << "CLIENT: " << reply << std::endl;

    return nullptr;
}

static void * worker_thread(void *arg)
{
    zmq::context_t context(1);
    zmq::socket_t worker(context, ZMQ_REQ);

    canna_setid(worker);
    worker.connect("tcp://localhost:5673");

    canna_send(worker, "READY");

    while (true) {
        std::string address = canna_recv(worker);
        {
            std::string empty = canna_recv(worker);
            assert(empty.size() == 0);
        }

        std::string request = canna_recv(worker);
        std::cout << "WORKER: " << request << std::endl;
        
        canna_sendmore(worker, address);
        canna_sendmore(worker, "");
        canna_send(worker, "OK");
    }

    return nullptr;
}

int main(int argv, char **argc)
{
    zmq::context_t context(1);
    zmq::socket_t frontend(context, ZMQ_ROUTER);
    zmq::socket_t backend(context, ZMQ_ROUTER);

    frontend.bind("tcp://*:5672");
    backend.bind("tcp://*:5673");

    int client_count;
    for (client_count = 0; client_count < 10; client_count++){
        pthread_t pid;
        pthread_create(&pid, nullptr, client_thread, (void *)(intptr_t)client_count);
    }

    int worker_count;
    for (worker_count = 0; worker_count < 3; worker_count++) {
        pthread_t pid;
        pthread_create(&pid, nullptr, worker_thread, (void *)(intptr_t)worker_count);
    }

    std::queue<std::string> worker_queue;
    while (true) {
        zmq::pollitem_t items[] = {
            { (void *)backend, 0, ZMQ_POLLIN, 0 }, 
            { (void *)frontend, 0, ZMQ_POLLIN, 0 }
        };

        if (worker_queue.size()) {
            zmq::poll(&items[0], 2, -1);
        } else {
            zmq::poll(&items[0], 1, -1);
        }

        if (items[0].revents & ZMQ_POLLIN) {
            worker_queue.push(canna_recv(backend));
            {
                std::string empty = canna_recv(backend);
                assert(empty.size() == 0);
            }
            std::string client_addr = canna_recv(backend);
            if (client_addr.compare("READY") != 0) {
                {
                    std::string empty = canna_recv(backend);
                    assert(empty.size() == 0);
                }
                std::string reply = canna_recv(backend);
                canna_sendmore(frontend, client_addr);
                canna_sendmore(frontend, "");
                canna_send(frontend, reply);
                if (--client_count == 0)
                    break;
            }
        }

        if (items[1].revents & ZMQ_POLLIN) {
            std::string client_addr = canna_recv(frontend);
            {
                std::string empty = canna_recv(frontend);
                assert(empty.size() == 0);
            }

            std::string request = canna_recv(frontend);
            std::string worker_addr = worker_queue.front();
            worker_queue.pop();

            canna_sendmore(backend, worker_addr);
            canna_sendmore(backend, "");
            canna_sendmore(backend, client_addr);
            canna_sendmore(backend, "");
            canna_send(backend, request);
        }
    }

    return 0;
}
