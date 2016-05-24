#ifndef _SOCKET_SERVER_H
#define _SOCKET_SERVER_H

#include "socket_poll.h"
#include "socket_define.h"
#include <stdlib.h>

struct socket_message {
    int id;
    uintptr_t opaque;
    int ud;
    char * data;
};

struct write_buffer {
    struct write_buffer * next;
    void * buffer;
    char * ptr;
    int sz;
    bool userobject;
};

#define SIZEOF_TCPBUFFER (sizeof(write_buffer))

struct wb_list {
    struct write_buffer * head;
    struct write_buffer * tail;

    void wb_clear() { head = tail = nullptr; }
    void wb_free() {
        struct write_buffer * wb = head;
        while (wb) {
            struct write_buffer * tmp = wb;
            wb = wb->next;
            free(tmp->buffer);
            free(tmp);
        }
        head = nullptr;
        tail = nullptr;
    }
};

struct socket {
    uintptr_t opaque;
    struct wb_list high;
    struct wb_list low;
    int64_t wb_size;
    int fd;
    int id;
    uint16_t protocol;
    uint16_t type;
    int size;
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

union sockaddr_all {
    struct sockaddr s;
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
};

class socket_server
{
    public:
        socket_server() {}
        ~socket_server() {}

        void initialize();
        void release();

        int server_listen(uintptr_t opaque, const char * addr, int port, int backlog);
        void server_start(uintptr_t opaque, int id);
        int server_bind(uintptr_t opaque, int fd);
        int server_connect(uintptr_t opaque, const char * addr, int port);
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
        int start_socket(struct request_start * request, struct socket_message * result);
        int bind_socket(struct request_bind * request, struct socket_message * result);
        int open_request(struct request_package * req, uintptr_t opaque, const char * addr, int port);
        int open_socket(struct request_open * request, struct socket_message * result);
        int close_socket(struct request_close * request, struct socket_message * result);

        void keepalive(int fd);
        void nonblocking(int fd);
        void setopt_socket(struct request_setopt * request);
        void force_close(struct socket * s, socket_message * result);

        int report_accept(struct socket *s, struct socket_message *result);
        int report_connect(struct socket *s, struct socket_message *result);
        void clear_closed_event(struct socket_message * result, int type);

        int send_list(struct socket *s, struct wb_list *list, struct socket_message *result);
        int send_list_tcp(struct socket *s, struct wb_list *list, struct socket_message *result);
        int list_uncomplete(struct wb_list * s);
        void raise_uncomplete(struct socket * s);
        int send_buffer(struct socket * s, struct socket_message * result);
        int send_buffer_empty(struct socket * s) { return (s->high.head == nullptr && s->low.head == nullptr); }

        int forward_message_tcp(struct socket * s, struct socket_message * result);
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
