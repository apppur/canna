#include <stdio.h>
#include <string>
#include "zmq.hpp"
#include "canna_core.h"
#include "canna_random.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("usage: peering me {you} ...\n");
        return 0;
    }

    char *self = argv[1];
    printf("I: preparing broker at %s ... \n", self);
    CaRandom random;
    random.SetSeed(canna_gettime());

    zmq::context_t context(1);

    zmq::socket_t statebe(context, ZMQ_PUB);
    char statebe_ipc[64];
    memset(statebe_ipc, 0, sizeof(statebe_ipc));
    sprintf(statebe_ipc, "ipc://%s-state.ipc", self);
    statebe.bind(statebe_ipc);

    zmq::socket_t statefe(context, ZMQ_SUB);
    const char *filter = "";
    statefe.setsockopt(ZMQ_SUBSCRIBE, filter, strlen(filter));
    for (int index = 2; index < argc; index++) {
        char *peer = argv[index];
        char statefe_ipc[64];
        memset(statefe_ipc, 0, sizeof(statefe_ipc));
        sprintf(statebe_ipc, "ipc://%s-state.ipc", peer);
        printf("I: connecting to state backend at '%s'\n", statefe_ipc);
        statefe.connect(statefe_ipc);
    }

    while (true) {
        zmq::pollitem_t items[] = {(void *)statefe, 0, ZMQ_POLLIN, 0};
        zmq::poll(items, 1, 1000);
        
        if (items[0].revents & ZMQ_POLLIN) {
            std::string peer_name = canna_recv(statefe);
            std::string available = canna_recv(statefe);
            printf("%s - %s work free \n", peer_name.c_str(), available.c_str());
        } else {
            canna_send(statebe, self);
            canna_send(statebe, "ping");
        }
    }

    return 0;
}
