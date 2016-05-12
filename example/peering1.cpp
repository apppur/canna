#include <pthread.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <queue>
#include "zmq.hpp"
#include "canna_core.h"

#define CLIENT_NUM 1
#define WORKER_NUM 1
#define WORKER_READY "READY"

static char *self;

static void * client_task(void *args)
{
    zmq::context_t context(1);
    zmq::socket_t client(context, ZMQ_REQ);
    char client_ipc[64] = {0};
    sprintf(client_ipc, "ipc://%s-localfe.ipc", self);
    client.connect(client_ipc);

    while (true) {
        canna_send(client, "HELLO");
        std::string reply = canna_recv(client);
        printf("CLIENT: %s\n", reply.c_str());
        canna_sleep(1000);
    }

    return nullptr;
}

static void * worker_task(void *args)
{
    zmq::context_t context(1);
    zmq::socket_t worker(context, ZMQ_REQ);
    char worker_ipc[64] = {0};
    sprintf(worker_ipc, "ipc://%s-localbe.ipc", self);
    worker.connect(worker_ipc);

    canna_send(worker, WORKER_READY);

    while (true) {
        std::string reply = canna_recv(worker);
        printf("WORKER: %s\n", reply.c_str());
        canna_send(worker, reply);
    }

    return nullptr;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: peering1 me {you}...\n");
        return 0;
    }
    self = argv[1];
    printf("I: preparing broker at: %s\n", self);

    zmq::context_t context(1);

    zmq::socket_t cloudfe(context, ZMQ_ROUTER);
    cloudfe.setsockopt(ZMQ_IDENTITY, self, strlen(self));
    char cloudfe_ipc[64] = {0};
    sprintf(cloudfe_ipc, "ipc://%s-cloud.ipc", self);
    cloudfe.bind(cloudfe_ipc);
    
    zmq::socket_t cloudbe(context, ZMQ_ROUTER);
    cloudbe.setsockopt(ZMQ_IDENTITY, self, strlen(self));
    for (int i = 2; i < argc; i++) {
        char *peer = argv[i];
        char peer_ipc[64] = {0};
        sprintf(peer_ipc, "ipc://%s-cloud.ipc", peer_ipc);
        printf("I: connecting to cloud frontend at %s \n", peer);
        cloudbe.connect(peer_ipc);
    }

    zmq::socket_t localfe(context, ZMQ_ROUTER);
    char localfe_ipc[64] = {0};
    sprintf(localfe_ipc, "ipc://%s-localfe.ipc", self);
    localfe.bind(localfe_ipc);

    zmq::socket_t localbe(context, ZMQ_ROUTER);
    char localbe_ipc[64] = {0};
    sprintf(localbe_ipc, "ipc://%s-localbe.ipc", self);
    localbe.bind(localbe_ipc);

    printf("Press enter when all broker are started: ");
    getchar();

    for (int i = 0; i < WORKER_NUM; i++) {
        pthread_t pid;
        pthread_create(&pid, nullptr, worker_task, nullptr);
    }

    for (int i = 0; i < CLIENT_NUM; i++) {
        pthread_t pid;
        pthread_create(&pid, nullptr, client_task, nullptr);
    }

    int capacity = 0;
    std::queue<std::string> worker_queue;
    while (true) {
        zmq::pollitem_t pollset[] = {
            {(void *)localbe, 0, ZMQ_POLLIN, 0},
            {(void *)cloudbe, 0, ZMQ_POLLIN, 0}
        };

        zmq::poll(pollset, 2, capacity ? 1000 : -1);

        if (pollset[0].revents & ZMQ_POLLIN) {
            std::string msg = canna_recv(localbe);
            
            if (msg == WORKER_READY) {
            } else
            {
                canna_send(localfe, msg);
            }
        }

        while (capacity) {
            zmq::pollitem_t frontends[] = {
                {(void *)localfe, 0, ZMQ_POLLIN, 0},
                {(void *)cloudfe, 0, ZMQ_POLLIN, 0}
            };
            zmq::poll(frontends, 2, 0);
            int reroutable = 0;
            std::string msg;
            if (frontends[1].revents & ZMQ_POLLIN) {
                msg = canna_recv(cloudfe);
                reroutable = 0;
            } else if (frontends[0].revents & ZMQ_POLLIN) {
                msg = canna_recv(localfe);
                reroutable = 1;
            } else {
                break;
            }

            if (reroutable) {
                canna_send(localbe, msg);
                reroutable--;
            }
        }
    }

    return 0;
}
