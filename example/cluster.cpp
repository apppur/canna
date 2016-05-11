#include <pthread.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <queue>
#include "canna_core.h"
#include "canna_random.h"
#include "zmq.hpp"

#define CLIENTS_NUM 10
#define WORKERS_NUM 5
#define WORKER_READY "READY"

static char *self;
static int serial = 0;

static void * client_task(void *args)
{
    zmq::context_t context(1);

    zmq::socket_t client(context, ZMQ_REQ);
    char client_ipc[64] = {0};
    sprintf(client_ipc, "ipc://%s-localfe.ipc", self);
    client.connect(client_ipc);

    zmq::socket_t monitor(context, ZMQ_PUSH);
    char monitor_ipc[64] = {0};
    sprintf(monitor_ipc, "ipc://%s-monitor.ipc", self);
    monitor.connect(monitor_ipc);

    while (true) {
        canna_sleep(CANNA_RAND(5));
        int burst = CANNA_RAND(15);
        while (burst--) {
            canna_send(client, "WORK");

            zmq::pollitem_t pollset[1] = {{(void *)client, 0, ZMQ_POLLIN, 0}};
            zmq::poll(pollset, 1, -1);

            if (pollset[0].revents & ZMQ_POLLIN) {
                std::string reply = canna_recv(client);
                canna_send(monitor, reply);
            } else {
                canna_send(monitor, "E: CLIENT EXIT - lost task");
            }
        }
    }

    return nullptr;
}

static void * worker_task(void *args)
{
    zmq::context_t context(1);
    zmq::socket_t work(context, ZMQ_REQ);
    //char uuid[16] = {0};
    //sprintf(uuid, "work:%d", serial++);
    //work.setsockopt(ZMQ_IDENTITY, uuid, 16);
    char work_ipc[64] = {0};
    sprintf(work_ipc, "ipc://%s-localbe.ipc", self);
    work.connect(work_ipc);

    canna_send(work, WORKER_READY);

    while (true) {
        std::string msg = canna_recv(work);
        canna_sleep(CANNA_RAND(2));
    }

    return nullptr;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: cluster me {you} \n");
        return 0;
    }
    self = argv[1];
    printf("I: preparing broker at %s...\n", self);

    zmq::context_t context(1);

    zmq::socket_t localfe(context, ZMQ_ROUTER);
    char localfe_ipc[64] = {0};
    sprintf(localfe_ipc, "ipc://%s-localfe.ipc", self);
    localfe.bind(localfe_ipc);

    zmq::socket_t localbe(context, ZMQ_ROUTER);
    char localbe_ipc[64] = {0};
    sprintf(localbe_ipc, "ipc://%s-localbe.ipc", self);
    localbe.bind(localbe_ipc);

    zmq::socket_t cloudfe(context, ZMQ_ROUTER);
    cloudfe.setsockopt(ZMQ_IDENTITY, self, strlen(self));
    char cloudfe_ipc[64] = {0};
    sprintf(cloudfe_ipc, "ipc://%s-cloud.ipc", self);
    cloudfe.bind(cloudfe_ipc);

    zmq::socket_t cloudbe(context, ZMQ_ROUTER);
    cloudbe.setsockopt(ZMQ_IDENTITY, self, strlen(self));
    char cloudbe_ipc[64] = {0};
    sprintf(cloudbe_ipc, "ipc://%s-cloudbe.ipc", self);
    for (int i = 2; i < argc; i++) {
        char *peer = argv[i];
        char peer_ipc[64] = {0};
        sprintf(peer_ipc, "ipc://%s-cloud.ipc", peer);
        printf("I: connecting to cloud frontend at '%s'\n", peer);
        cloudbe.connect(peer_ipc);
    }

    zmq::socket_t statebe(context, ZMQ_PUB);
    char statebe_ipc[64] = {0};
    sprintf(statebe_ipc, "ipc://%s-state.ipc", self);
    statebe.bind(statebe_ipc);

    zmq::socket_t statefe(context, ZMQ_SUB);
    const char *filter = "";
    statefe.setsockopt(ZMQ_SUBSCRIBE, filter, strlen(filter));
    for (int i = 2; i < argc; i++) {
        char *peer = argv[i];
        char peer_ipc[64] = {0};
        sprintf(peer_ipc, "ipc://%s-state.ipc", peer);
        statefe.connect(peer_ipc);
    }

    zmq::socket_t monitor(context, ZMQ_PULL);
    char monitor_ipc[64] = {0};
    sprintf(monitor_ipc, "ipc://%s-monitor.ipc", self);
    monitor.bind(monitor_ipc);

    for (int i = 0; i < WORKERS_NUM; i++) {
        pthread_t pid;
        pthread_create(&pid, nullptr, worker_task, nullptr);
    }

    for (int i = 0; i < CLIENTS_NUM; i++) {
        pthread_t pid;
        pthread_create(&pid, nullptr, client_task, nullptr);
    }

    std::queue<std::string> worker_queue;
    while (true) {
        zmq::pollitem_t pollset[] = {
            {(void *)localbe, 0, ZMQ_POLLIN, 0},
            {(void *)cloudbe, 0, ZMQ_POLLIN, 0},
            {(void *)statefe, 0, ZMQ_POLLIN, 0},
            {(void *)monitor, 0, ZMQ_POLLIN, 0},
        };

        if (worker_queue.size()) {
            zmq::poll(pollset, 4, -1);
        } else {
            zmq::poll(pollset, 4, 1000);
        }

        if (pollset[0].revents & ZMQ_POLLIN) {
            canna_dump(localbe);
            std::string identity = canna_recv(localbe);
            worker_queue.push(identity);
            std::string empty = canna_recv(localbe);
            std::string msg = canna_recv(localbe);
            printf("%s: %s\n", identity.c_str(), msg.c_str());
        } else if (pollset[1].revents & ZMQ_POLLIN) {
        }

        if (pollset[2].revents & ZMQ_POLLIN) {
        }

        if (pollset[3].revents & ZMQ_POLLIN) {
            std::string status = canna_recv(monitor);
            printf("%s\n", status.c_str());
        }
    }
}
