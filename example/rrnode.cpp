#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include "zmq.hpp"
#include "canna_core.h"

static void * work_thread(void * arg) {
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_ROUTER);
    
    socket.setsockopt(ZMQ_IDENTITY, "APPPLE", 6);
    socket.connect("tcp://localhost:5555");

    canna_sleep(5000);

    canna_sendmore(socket, "PURPLE");
    canna_sendmore(socket, "");
    canna_send(socket, "Hello World!!!");

    while (true) {
        canna_sleep(5000);
    }

    return nullptr;
}

int main(int argc, char** argv)
{
    zmq::context_t context(1);
    
    zmq::socket_t sink(context, ZMQ_ROUTER);
    //zmq::socket_t sink(context, ZMQ_REP);
    sink.setsockopt(ZMQ_IDENTITY, "PURPLE", 6);
    sink.bind("tcp://*:5555");

    /*
    zmq::socket_t apple(context, ZMQ_ROUTER);
    apple.setsockopt(ZMQ_IDENTITY, "APPPLE", 6);
    apple.connect("tcp://localhost:5555");

    canna_sendmore(apple, "PURPLE");
    canna_sendmore(apple, "");
    canna_send(apple, "ROUTER socket use REQ's identity");
    //canna_dump(sink);
    std::string identity = canna_recv(sink);
    std::string empty = canna_recv(sink);
    std::string reply = canna_recv(sink);
    printf("RECV: %s\n", reply.c_str());
    printf("*******************************************\n");
    */

    //pthread_t pid;
    //pthread_create(&pid, nullptr, work_thread, nullptr);


    while (true) {
        printf("************************************\n");
        std::string identity = canna_recv(sink);
        printf("IDENTITY: %s\n", identity.c_str());
        std::string reply = canna_recv(sink);
        printf("REPLY: %s\n", reply.c_str());
        printf("************************************\n");

        canna_sendmore(sink, "APPPLE");
        canna_send(sink, "ACK");

        canna_sleep(2000);
    }

    return 0;
}
