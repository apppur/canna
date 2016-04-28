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

#define SERVER_INIT_DAEMON 0
#define COMMAND_LEN 256

char command[COMMAND_LEN];

int initialize(int mode);
int initdaemon(int mode);
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

int initdaemon(int mode)
{
    if (SERVER_INIT_DAEMON == mode)
    {
        return 0;
    }

    pid_t pid;
    if ((pid = fork()) != 0)
    {
        exit(0);
    }

    setsid();

    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    struct sigaction sig;
    sig.sa_handler = SIG_IGN;
    sig.sa_flags = 0;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGHUP, &sig, NULL);

    if ((pid == fork()) != 0)
    {
        exit(0);
    }

    int pid_fd = open("./dbserver.pid", O_RDWR|O_CREAT, 0644);
    if (pid_fd < 0)
    {
        printf("open pid file failed, dbserver init failed\n");
        return -1;
    }
    FILE* pfile = fdopen(pid_fd, "w+");
    if (pfile == NULL)
    {
        printf("fdopen pid file failed, dbserver init failed\n");
        return -2;
    }
    if (flock(pid_fd, LOCK_EX|LOCK_NB) == -1)
    {
        printf("lock pid file failed, dbserver is already running\n");
        return -3;
    }
    pid = getpid();
    if (!fprintf(pfile, "%d\n", pid))
    {
        printf("write pid to file failed\n");
        close(pid_fd);
        return -4;
    }
    fflush(pfile);

    umask(0);
    setpgrp();

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
    result = initdaemon(mode);

    if (0 != result)
    {
        exit(0);
    }

    signal(SIGUSR1, signal_usr1);
    signal(SIGUSR2, signal_usr2);

    return result;
}
