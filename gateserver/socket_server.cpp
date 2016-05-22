#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "socket_server.h"

void socket_server::initialize()
{
    event_fd.create();
    if (event_fd.invalid()) {
        fprintf(stderr, "socket-server: create event pool failed.\n");
        return;
    }
    int fd[2];
    if (pipe(fd)) {
        event_fd.release();
        fprintf(stderr, "socket-server: create socket pair failed.\n");
        return;
    }
    if (event_fd.add(fd[0], nullptr)) {
        fprintf(stderr, "socket-server: can't add server fd to event pool.\n");
        close(fd[0]);
        close(fd[1]);
        event_fd.release();
        return;
    }

    recvctrl_fd = fd[0];
    sendctrl_fd = fd[1];
    checkctrl = 1;

    for (int i = 0; i < MAX_SOCKET; i++) {
        slot[i].type = SOCKET_TYPE_INVALID;
    }

    alloc_id = 0;
    event_n = 0;
    event_index = 0;
    
    FD_ZERO(&rfds);

    assert(recvctrl_fd < FD_SETSIZE);

    return;
}

void socket_server::release()
{
    close(sendctrl_fd);
    close(recvctrl_fd);
    event_fd.release();
}

int socket_server::reserve_id()
{
    for (int i = 0; i < MAX_SOCKET; i++) {
        int id = __sync_add_and_fetch(&alloc_id, 1);
        if (id < 0) {
            id = __sync_and_and_fetch(&alloc_id, 0x7fffffff);
        }
        struct socket *s = &slot[HASH_ID(id)];

        if (s->type == SOCKET_TYPE_INVALID) {
            if (__sync_bool_compare_and_swap(&s->type, SOCKET_TYPE_INVALID, SOCKET_TYPE_RESERVE)) {
                s->id = id;
                s->fd = -1;
                return id;
            } else {
                --i;
            }
        }
    }
    return -1;
}

int socket_server::do_bind(const char *host, int port, int protocol, int *family)
{
    int fd, status;
    int reuse = 1;
    struct addrinfo ai_hints;
    struct addrinfo *ai_list = nullptr;
    char portstr[16];
    if (host == nullptr || host[0] == 0) {
        host = "0.0.0.0";
    }
    sprintf(portstr, "%d", port);
    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_family = AF_UNSPEC;
    if (protocol == IPPROTO_TCP) {
        ai_hints.ai_socktype = SOCK_STREAM;
    } else {
        assert(protocol == IPPROTO_UDP);
        ai_hints.ai_socktype = SOCK_DGRAM;
    }
    ai_hints.ai_protocol = protocol;

    status = getaddrinfo(host, portstr, &ai_hints, &ai_list);
    if (status != 0) {
        return -1;
    }
    *family = ai_list->ai_family;
    fd = socket(*family, ai_list->ai_socktype, 0);
    if (fd < 0) {
        goto _failed_fd;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(int)) == -1) {
        goto _failed;
    }
    status = bind(fd, (struct sockaddr *)ai_list->ai_addr, ai_list->ai_addrlen);
    if (status != 0) {
        goto _failed;
    }
    freeaddrinfo(ai_list);
    return fd;
    
_failed:
    close(fd);
_failed_fd:
    freeaddrinfo(ai_list);
    return -1;
}

int socket_server::do_listen(const char * host, int port, int backlog)
{
    int family = 0;
    int listen_fd = do_bind(host, port, IPPROTO_TCP, &family);
    if (listen_fd < 0) {
        return -1;
    }
    if (listen(listen_fd, backlog) == -1) {
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

void socket_server::send_request(struct request_package *request, char type, int len)
{
    request->header[6] = (uint8_t)type;
    request->header[7] = (uint8_t)len;
    printf("send request type: %c, len:%d\n", type, len);
    for ( ; ; ) {
        int n = write(sendctrl_fd, &request->header[6], len+2);
        if (n < 0) {
            if (errno != EINTR) {
                fprintf(stderr, "socket-server: send ctrl command error %s.\n", strerror(errno));
            }
            continue;
        }
        assert(n == len+2 );
        return;
    }
}

int socket_server::server_listen(uintptr_t opaque, const char *addr, int port, int backlog)
{
    int fd = do_listen(addr, port, backlog);
    if (fd < 0) {
        printf("do listen failed\n");
        return -1;
    }
    printf("run listen success!\n");

    struct request_package request;
    int id = reserve_id();
    if (id < 0) {
        close(fd);
        return id;
    }
    request.u.listen.opaque = opaque;
    request.u.listen.id = id;
    request.u.listen.fd = fd;
    send_request(&request, 'L', sizeof(request.u.listen));
    return id;
}

void socket_server::block_readpipe(int pipefd, void *buffer, int sz) 
{
    for ( ; ; ) {
        int n = read(pipefd, buffer, sz);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "socket-server: read pipe error %s.\n", strerror(errno));
            return;
        }
        printf("The size: %d, %d\n", n, sz);
        assert(n == sz);
        return;
    }
}

int socket_server::has_cmd() {
    struct timeval tv = {0, 0};
    FD_SET(recvctrl_fd, &rfds);
    int retval = select(recvctrl_fd+1, &rfds, nullptr, nullptr, &tv);
    if (retval == 1) {
        return 1;
    }
    return 0;
}

int socket_server::ctrl_cmd(struct socket_message * result) 
{
    int fd = recvctrl_fd;
    uint8_t buffer[256];
    uint8_t header[2];
    block_readpipe(fd, (void *)header, sizeof(header));
    int type = header[0];
    int len = header[1];
    block_readpipe(fd, buffer, len);

    switch (type) {
        case 'S':
            break;
        case 'B':
            break;
        case 'L':
            printf("**************start to listen**************** \n");
            return listen_socket((struct request_listen *)buffer, result);
        case 'K':
            break;
        case 'O':
            break;
        case 'X':
            break;
        case 'D':
            break;
        case 'P':
            break;
        case 'A':
            break;
        case 'C':
            break;
        case 'T':
            break;
        case 'U':
            break;
        default:
            fprintf(stderr, "socket-server: unknown ctrl %c.\n", type);
            break;
    }
    return -1;
}

int socket_server::poll(struct socket_message * result, int * more)
{
    for ( ; ; ) {
        if (checkctrl) {
            if (has_cmd()) {
                int type = ctrl_cmd(result);
                if (type != -1) {
                    return type;
                } else 
                {
                    continue;
                }
            } else {
                checkctrl = 0;
            }
        }
        if (event_index == event_n) {
            event_n = event_fd.wait(ev, MAX_EVENT);
            checkctrl = 1;
            if (more) {
                *more = 0;
            }
            event_index = 0;
            if (event_n <= 0) {
                event_n = 0;
                return -1;
            }
        }
        struct event * e = &ev[event_index++];
        struct socket * s = (struct socket *)e->s;
        if (s == nullptr) {
            continue;
        }
        switch (s->type) {
            case SOCKET_TYPE_CONNECTING:
                break;
            case SOCKET_TYPE_LISTEN:
                printf("********new client connect*********\n");
                if (report_accept(s, result)) {
                    return SOCKET_ACCEPT;
                }
                break;
            default:
                break;
        }
    }
}

struct socket * socket_server::new_socket(int id, int fd, int protocol, uintptr_t opaque, bool add)
{
    struct socket * s = &slot[HASH_ID(id)];
    assert(s->type == SOCKET_TYPE_RESERVE);

    if (add) {
        if (event_fd.add(fd, s)) {
            s->type = SOCKET_TYPE_INVALID;
            return nullptr;
        }
    }

    s->id = id;
    s->fd = fd;
    s->protocol = protocol;

    s->opaque = opaque;

    return s;
}

int socket_server::listen_socket(struct request_listen * request, struct socket_message * result)
{
    int id = request->id;
    int listen_fd = request->fd;
    struct socket * s = new_socket(id, listen_fd, PROTOCOL_TCP, request->opaque, false);
    if (s == nullptr) {
        goto _failed;
    }
    s->type = SOCKET_TYPE_PLISTEN;
    return -1;
_failed:
    close(listen_fd);
    result->opaque = request->opaque;
    result->id = id;
    result->ud = 0;
    result->data = nullptr;
    slot[HASH_ID(id)].type = SOCKET_TYPE_INVALID;
     
    return SOCKET_ERROR;
}


void socket_server::keepalive(int fd)
{
    int keepalive = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
}

void socket_server::nonblocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (-1 == flag) {
        return;
    }

    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

int socket_server::report_accept(struct socket *s, struct socket_message *result)
{
    union sockaddr_all u;
    socklen_t len = sizeof(u);
    int client_fd = accept(s->fd, &u.s, &len);
    if (client_fd < 0) {
        return 0;
    }
    int id = reserve_id();
    if (id < 0) {
        close(client_fd);
        return 0;
    }
    keepalive(client_fd);
    nonblocking(client_fd);

    struct socket *ns = new_socket(id, client_fd, PROTOCOL_TCP, s->opaque, false);
    if (ns == nullptr) {
        close(client_fd);
        return 0;
    }
    ns->type = SOCKET_TYPE_PACCEPT;
    result->opaque = s->opaque;
    result->id = s->id;
    result->ud = id;
    result->data = nullptr;

    void * sin_addr = (u.s.sa_family == AF_INET) ? (void *)&u.v4.sin_addr : (void *)&u.v6.sin6_addr;
    int sin_port = ntohs((u.s.sa_family == AF_INET) ? u.v4.sin_port : u.v6.sin6_port);
    char tmp[INET6_ADDRSTRLEN];
    if (inet_ntop(u.s.sa_family, sin_addr, tmp, sizeof(tmp))) {
        snprintf(buffer, sizeof(buffer), "%s:%d", tmp, sin_port);
        result->data = buffer;
    }

    return 1;
}
