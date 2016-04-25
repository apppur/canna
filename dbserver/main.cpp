#include <stdio.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <assert.h>
#include "hiredis.h"
#include "zmq.hpp"

int main(int argc, char **argv)
{
    redisContext *context = redisConnect("127.0.0.1", 6379);
    if (context == nullptr || context->err) {
        if (context) {
            printf("Error: %s, Code: %d\n", context->errstr, context->err);
        } else {
            printf("Can't allocate redis context\n");
        }
    } else {
        printf("Success: context init \n");
    }

    redisReply *reply = (redisReply *)redisCommand(context, "SET foo barbarbar");


        //  Prepare our context and socket
        zmq::context_t zmq_context (1);
        zmq::socket_t socket (zmq_context, ZMQ_REP);
        socket.bind ("tcp://*:5555");
        
        while (true) {
            zmq::message_t request;

            //  Wait for next request from client
            socket.recv (&request);
            std::cout << "Received Hello" << std::endl;

            //  Do some 'work'
            sleep(1);

            //  Send reply back to client
            zmq::message_t reply (5);
            memcpy (reply.data(), "World", 5);
            socket.send(reply);
        }
        return 0;
}
