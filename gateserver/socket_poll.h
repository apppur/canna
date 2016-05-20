#ifndef _SOCKET_POLL_H
#define _SOCKET_POLL_H

typedef int poll_fd;

struct event {
    void * s;
    bool read;
    bool write;
};

#ifdef __linux__
#include "socket_epoll.h"
#endif

#endif
