#include <pthread.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <queue>
#include "zmq.hpp"
#include "canna_core.h"

#define CLIENT_NUM 5
#define WORKER_NUM 3
#define WORKER_READY "READY"

static char *self;
static char *cloud;

static void * client_task(void *args)
{
    printf("client thread start\n");
    zmq::context_t context(1);
    zmq::socket_t client(context, ZMQ_REQ);
    char client_ipc[64] = {0};
    sprintf(client_ipc, "ipc://%s-localfe.ipc", self);
    client.connect(client_ipc);

    while (true) {
        canna_send(client, "HELLO");
        std::string reply = canna_recv(client);
        printf("CLIENT: %s\n", reply.c_str());
        //canna_sleep(1000);
    }

    return nullptr;
}

static void * worker_task(void *args)
{
    printf("worker thread start\n");
    zmq::context_t context(1);
    zmq::socket_t worker(context, ZMQ_REQ);
    char worker_ipc[64] = {0};
    sprintf(worker_ipc, "ipc://%s-localbe.ipc", self);
    printf("worker task ipc: %s\n", worker_ipc);
    worker.connect(worker_ipc);

    //canna_sleep(1000);

    canna_send(worker, WORKER_READY);

    while (true) {
        std::string address = canna_recv(worker);
        std::string empty = canna_recv(worker);
        std::string reply = canna_recv(worker);
        printf("WORKER: %s\n", reply.c_str());
        canna_sendmore(worker, address);
        canna_sendmore(worker, "");
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
        cloud = argv[i];
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
    printf("localbe ipc: %s\n", localbe_ipc);
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

        zmq::poll(pollset, 2, 0);

        if (pollset[0].revents & ZMQ_POLLIN) {
            std::string work_identity = canna_recv(localbe);
            std::string empty = canna_recv(localbe);
            worker_queue.push(work_identity);
            capacity++;
            
            printf("///////////////////////////////////\n");
            std::string client_addr = canna_recv(localbe);
            if (client_addr == WORKER_READY) {
            } else
            {
                {
                    std::string empty = canna_recv(localbe);
                }
                std::string reply = canna_recv(localbe);
                printf("===============================:%s\n", reply.c_str());
                canna_sendmore(localfe, client_addr);
                canna_sendmore(localfe, "");
                canna_send(localfe, "SUCCESS");
            }
        }

        if (pollset[1].revents & ZMQ_POLLIN) {
            printf("***********************************************************");
            std::string cloud_identity = canna_recv(cloudbe);
            std::string empty = canna_recv(cloudbe);
            std::string reply = canna_recv(cloudbe);
            printf("***********************************************************");
            printf("CLOUD INFO: %s, %s\n", cloud_identity.c_str(), reply.c_str());
            printf("***********************************************************");
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
                printf("***********************************************************");
                std::string cloud_identity = canna_recv(cloudfe);
                std::string empty = canna_recv(cloudfe);
                std::string reply = canna_recv(cloudfe);
                printf("***********************************************************");
                printf("CLOUD INFO: %s, %s\n", cloud_identity.c_str(), reply.c_str());
                printf("***********************************************************");
            }
            if (frontends[0].revents & ZMQ_POLLIN) {
                std::string identity = canna_recv(localfe);
                std::string empty = canna_recv(localfe);
                std::string reply = canna_recv(localfe);
                printf("RECV msg: %s from client, capacity: %d\n", reply.c_str(), capacity);
                reroutable = 1;
            
                //if (CANNA_RAND(5) == 0) {
                    canna_sendmore(cloudbe, cloud);
                    canna_sendmore(cloudbe, "");
                    canna_send(cloudbe, "CLOUD");
                //} else {
                    std::string worker_addr = worker_queue.front();
                    worker_queue.pop();
                    canna_sendmore(localbe, worker_addr);
                    canna_sendmore(localbe, "");
                    canna_sendmore(localbe, identity);
                    canna_sendmore(localbe, "");
                    canna_send(localbe, "WORLD");
                    reroutable--;
                    capacity--;
                //}
            } else {
                break;
            }

            /*
            if (reroutable) {
                canna_sendmore(localbe, identity);
                canna_sendmore(localbe, "");
                canna_send(localbe, "WORLD");
                reroutable--;
                capacity--;
            }
            */
        }
    }

    return 0;
}
