#ifndef _SOCKET_SERVER_H
#define _SOCKET_SERVER_H

#include "socket_poll.h"
#include "socket_define.h"

struct socket_message {
    int id;
    uintptr_t opaque;
    int ud;
    char * data;
};

struct socket {
    uintptr_t opaque;
    int64_t wb_size;
    int fd;
    int id;
    uint16_t protocol;
    uint16_t type;
};

struct request_open {
    int id;
    int port;
    uintptr_t opaque;
    char host[1];
};

struct request_send {
    int id;
    int sz;
    char *buffer;
};

struct request_close {
    int id;
    uintptr_t opaque;
};

struct request_listen {
    int id;
    int fd;
    uintptr_t opaque;
    char host[1];
};

struct request_bind {
    int id;
    int fd;
    uintptr_t opaque;
};

struct request_start {
    int id;
    uintptr_t opaque;
};

struct request_setopt {
    int id;
    int what;
    int value;
};

/*
   The first byte is TYPE
   S Start socket
   B Bind socket
   L Listen socket
   K Close socket
   O Connect to (Open)
   X Exit
   D Send package (high)
   P Send package (low)
   A Send UDP package
   T Set opt
   U Create UDP socket
   C set udp address
   */
struct request_package {
    uint8_t header[8];
    union {
        char buffer[256];
        struct request_open open;
        struct request_send send;
        struct request_close close;
        struct request_listen listen;
        struct request_bind bind;
        struct request_start start;
        struct request_setopt setopt;
    } u;
    uint8_t dummy[256];
};

class socket_server
{
    public:
        socket_server() {}
        ~socket_server() {}

        void initialize();
        void release();

        int server_listen(uintptr_t opaque, const char * addr, int port, int backlog);
        int poll(struct socket_message * result, int *more);

    private:
        int do_bind(const char *host, int port, int protocol, int *family);
        int do_listen(const char *host, int port, int backlog);
        void send_request(struct request_package *request, char type, int len);

        int reserve_id();

        void block_readpipe(int pipefd, void *buffer, int sz);
        int has_cmd();
        int ctrl_cmd(struct socket_message * result);

        struct socket * new_socket(int id, int fd, int protocol, uintptr_t opaque, bool add);
        int listen_socket(struct request_listen * request, struct socket_message * result);
    private:
        int recvctrl_fd;
        int sendctrl_fd;
        int checkctrl;
        //poll_fd event_fd;
        socket_poll event_fd;
        int alloc_id;
        int event_n;
        int event_index;
        struct event ev[MAX_EVENT];
        struct socket slot[MAX_SOCKET];
        char buffer[MAX_INFO];
        fd_set rfds;
};

#endif
