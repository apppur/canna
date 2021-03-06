#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>

#include "redis_client.h"
#include "name_ctrl.h"
#include "name_server.h"
#include "canna_core.h"
#include "canna_daemon.h"

int initialize(int mode);
void signal_usr1(int signal);
void signal_usr2(int signal);

int main(int argc, char ** argv)
{
    initialize(DAEMON_MODE);

    /*
    redis_client redisclient;
    redisclient.initialize("127.0.0.1", 6379);
    if (redisclient.connect() != 0) {
        exit(0);
    }
    */

    name_server nameserver;
    nameserver.initialize();
    nameserver.responder_create();
    nameserver.responder_bind("tcp://*:5555");
    nameserver.publisher_create();
    nameserver.publisher_bind("tcp://*:5556");
    nameserver.server_loop();

    return 0;
}

int initialize(int mode)
{
    int result;
    result = daemon_init(mode, "./nameserver.pid");

    if (0 != result) {
        exit(0);
    }

    signal(SIGUSR1, signal_usr1);
    signal(SIGUSR2, signal_usr2);

    return result;
}

void signal_usr1(int signal)
{
}
void signal_usr2(int signal)
{
}
