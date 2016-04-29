#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <assert.h>
#include "hiredis.h"
#include "zmq.hpp"
#include "../core/canna_daemon.h"

#define COMMAND_LEN 256

char command[COMMAND_LEN];

int initialize(int mode);
void signal_usr1(int signal);
void signal_usr2(int signal);

int main(int argc, char **argv)
{
    initialize(1);

    int major, minor, patch;
    zmq::version(&major, &minor, &patch);
    printf("current zeromq version is %d.%d.%d\n", major, minor, patch);

    redisContext *context = redisConnect("127.0.0.1", 6379);
    if (context == nullptr || context->err) {
        if (context) {
            printf("Error: %s, Code: %d\n", context->errstr, context->err);
            return 1;
        } else {
            printf("Can't allocate redis context\n");
            return 1;
        }
    } else {
        printf("Success: context init \n");
    }

    //redisReply *reply = (redisReply *)redisCommand(context, "SET foo barbarbar");


    //  Prepare our context and socket
    zmq::context_t zmq_context (1);
    zmq::socket_t socket (zmq_context, ZMQ_REP);
    socket.bind ("tcp://*:5555");

    while (true) {
        zmq::message_t request;

        //  Wait for next request from client
        socket.recv (&request);
        if (request.size() > COMMAND_LEN) {
            continue;
        }

        memset(command, 0, COMMAND_LEN);
        memcpy(command, static_cast<char*>(request.data()), request.size());

        redisReply *result = (redisReply *)redisCommand(context, command);

        //  Send reply back to client
        zmq::message_t reply(result->len);
        memcpy (reply.data(), result->str, result->len);
        socket.send(reply);

        freeReplyObject(result);
    }
        return 0;
}

void signal_usr1(int signal)
{
}

void signal_usr2(int signal)
{
}

int initialize(int mode)
{
    int result;
    result = daemon_init(mode, "./dbserver.pid");

    if (0 != result)
    {
        exit(0);
    }

    signal(SIGUSR1, signal_usr1);
    signal(SIGUSR2, signal_usr2);

    return result;
}
