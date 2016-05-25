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
        slot[i].high.wb_clear();
        slot[i].low.wb_clear();
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
    struct socket_message dummy;
    for (int i = 0; i < MAX_SOCKET; i++) {
        struct socket * s = &slot[i];
        if (s->type != SOCKET_TYPE_RESERVE) {
            force_close(s, &dummy);
        }
    }
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
            return start_socket((struct request_start *)buffer, result);
        case 'B':
            return bind_socket((struct request_bind *)buffer, result);
        case 'L':
            return listen_socket((struct request_listen *)buffer, result);
        case 'K':
            return close_socket((struct request_close *)buffer, result);
        case 'O':
            return open_socket((struct request_open *)buffer, result);;
        case 'X':
            result->opaque = 0;
            result->ud = 0;
            result->id = 0;
            result->data = nullptr;
            return SOCKET_EXIT;
        case 'D':
            printf("*****************send data*******************\n");
            return send_socket((struct request_send *)buffer, result, PRIORITY_HIGH);
        case 'P':
            break;
        case 'A':
            break;
        case 'C':
            break;
        case 'T':
            setopt_socket((struct request_setopt *)buffer);
            return -1;
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
                    clear_closed_event(result, type);
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
                printf("********connect ing*********\n");
                return report_connect(s, result);
            case SOCKET_TYPE_LISTEN:
                printf("********new client connect*********\n");
                if (report_accept(s, result)) {
                    return SOCKET_ACCEPT;
                }
                break;
            case SOCKET_TYPE_INVALID:
                fprintf(stderr, "socket-server: invalid socket \n");
                break;
            default:
                if (e->read) {
                    int type = -1;
                    if (s->protocol == PROTOCOL_TCP) {
                        type = forward_message_tcp(s, result);
                    }
                    if (e->write) {
                        e->read = false;
                        --event_index;
                    }
                    if (type == -1) {
                        break;
                    }
                    clear_closed_event(result, type);
                    return type;
                }
                if (e->write) {
                    int type = send_buffer(s, result);
                    if (type == -1)
                        break;
                    clear_closed_event(result, type);
                    return type;
                }
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

void socket_server::server_start(uintptr_t opaque, int id)
{
    struct request_package request;
    request.u.start.id = id;
    request.u.start.opaque = opaque;
    send_request(&request, 'S', sizeof(request.u.start));
}

int socket_server::start_socket(struct request_start * request, struct socket_message * result)
{
    int id = request->id;
    result->id = id;
    result->opaque = request->opaque;
    result->ud = 0;
    result->data = nullptr;
    struct socket *s = &slot[HASH_ID(id)];
    if (s->type == SOCKET_TYPE_INVALID || s->id != id) {
        return SOCKET_ERROR;
    }
    if (s->type == SOCKET_TYPE_PACCEPT || s->type == SOCKET_TYPE_PLISTEN) {
        if (event_fd.add(s->fd, s)) {
            s->type = SOCKET_TYPE_INVALID;
            return SOCKET_ERROR;
        }
        s->type = (s->type == SOCKET_TYPE_PACCEPT) ? SOCKET_TYPE_CONNECTED : SOCKET_TYPE_LISTEN;
        s->opaque = request->opaque;
        result->data = (char *)"start";
        return SOCKET_OPEN;
    } else if (s->type == SOCKET_TYPE_CONNECTED) {
        s->opaque = request->opaque;
        result->data = (char *)"transfer";
        return SOCKET_OPEN;
    }

    return -1;
}

int socket_server::server_bind(uintptr_t opaque, int fd) 
{
    struct request_package request;
    int id = reserve_id();
    if (id < 0) {
        return -1;
    }
    request.u.bind.opaque = opaque;
    request.u.bind.id = id;
    request.u.bind.fd = fd;
    send_request(&request, 'B', sizeof(request.u.bind));
    return id;
}

int socket_server::bind_socket(struct request_bind * request, struct socket_message * result)
{
    int id = request->id;
    result->id = id;
    result->opaque = request->opaque;
    result->ud = 0;
    struct socket * s = new_socket(id, request->fd, PROTOCOL_TCP, request->opaque, true);
    if (s == nullptr) {
        result->data = nullptr;
        return SOCKET_ERROR;
    }
    nonblocking(request->fd);
    s->type = SOCKET_TYPE_BIND;
    result->data = (char *)"binding";
    return SOCKET_OPEN;
}

int socket_server::server_connect(uintptr_t opaque, const char * addr, int port)
{
    struct request_package request;
    int len = open_request(&request, opaque, addr, port);
    if (len < 0) {
        return -1;
    }
    send_request(&request, 'O', sizeof(request.u.open) + len);
    return request.u.open.id;
}

int socket_server::open_request(struct request_package * req, uintptr_t opaque, const char * addr, int port)
{
    int len = strlen(addr);
    if (len + sizeof(req->u.open) > 256) {
        fprintf(stderr, "socket-server : Invalid addr %s.\n", addr);
        return -1;
    }
    int id = reserve_id();
    if (id < 0) {
        return -1;
    }
    req->u.open.opaque = opaque;
    req->u.open.id = id;
    req->u.open.port = port;
    memcpy(req->u.open.host, addr, len);
    req->u.open.host[len] = '\0';

    return len;
}

int socket_server::open_socket(struct request_open * request, struct socket_message * result)
{
    int id = request->id;
    result->id = id;
    result->opaque = request->opaque;
    result->ud = 0;
    result->data = nullptr;
    struct socket *ns;
    int status;
    struct addrinfo ai_hints;
    struct addrinfo * ai_list = nullptr;
    struct addrinfo * ai_ptr = nullptr;
    char port[16];
    sprintf(port, "%d", request->port);
    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_STREAM;
    ai_hints.ai_protocol = IPPROTO_TCP;

    status = getaddrinfo(request->host, port, &ai_hints, &ai_list);
    int sock = -1;
    if (status != 0) {
        goto _failed;
    }
    for (ai_ptr = ai_list; ai_list != nullptr; ai_ptr = ai_ptr->ai_next) {
        sock = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
        if (sock < 0) {
            continue;
        }
        keepalive(sock);
        nonblocking(sock);
        status = connect(sock, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
        if (status != 0 && errno != EINPROGRESS) {
            close(sock);
            sock = -1;
            continue;
        }
        break;
    }

    if (sock < 0) {
        goto _failed;
    }

    ns = new_socket(id, sock, PROTOCOL_TCP, request->opaque, true);
    if (status == 0) {
        ns-> type = SOCKET_TYPE_CONNECTED;
        struct sockaddr * addr = ai_ptr->ai_addr;
        void * sin_addr = (ai_ptr->ai_family == AF_INET) ? (void *)&((struct sockaddr_in *)addr)->sin_addr : (void *)&((struct sockaddr_in6 *)addr)->sin6_addr;
        if (inet_ntop(ai_ptr->ai_family, sin_addr, buffer, sizeof(buffer))) {
            result->data = buffer;
        }
        freeaddrinfo(ai_list);
        return SOCKET_OPEN;
    } else {
        ns->type = SOCKET_TYPE_CONNECTING;
        event_fd.write(ns->fd, ns, true);
    }

    freeaddrinfo(ai_list);
    return -1;

_failed:
    freeaddrinfo(ai_list);
    slot[HASH_ID(id)].type = SOCKET_TYPE_INVALID;
    return SOCKET_ERROR;
}

int socket_server::report_connect(struct socket *s, struct socket_message *result)
{
    int error;
    socklen_t len = sizeof(error);
    int code = getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &error, &len);
    if (code < 0 || error) {
        force_close(s, result);
        return SOCKET_ERROR;
    } else {
        s->type = SOCKET_TYPE_CONNECTED;
        result->opaque = s->opaque;
        result->id = s->id;
        result->ud = 0;
        if (send_buffer_empty(s)) {
            event_fd.write(s->fd, s, false);
        }
        union sockaddr_all u;
        socklen_t slen = sizeof(u);
        if (getpeername(s->fd, &u.s, &slen)) {
            void * sin_addr = (u.s.sa_family == AF_INET) ? (void *)&u.v4.sin_addr : (void *)&u.v6.sin6_addr;
            if (inet_ntop(u.s.sa_family, sin_addr, buffer, sizeof(buffer))) {
                result->data = buffer;
                return SOCKET_OPEN;
            }
        }
        result->data = nullptr;
        return SOCKET_OPEN;
    }
}

void socket_server::force_close(struct socket * s, socket_message * result)
{
    result->id = s->id;
    result->ud = 0;
    result->data = nullptr;
    result->opaque = s->opaque;
    if (s->type == SOCKET_TYPE_INVALID) {
        return;
    }
    assert(s->type != SOCKET_TYPE_RESERVE);
    s->high.wb_clear();
    s->low.wb_clear();
    if (s->type != SOCKET_TYPE_PACCEPT && s->type != SOCKET_TYPE_PLISTEN) {
        event_fd.del(s->fd);
    }
    if (s->type != SOCKET_TYPE_BIND) {
        close(s->fd);
    }
    s->type = SOCKET_TYPE_INVALID;
    return;
}

int socket_server::send_list_tcp(struct socket *s, struct wb_list *list, struct socket_message *result)
{
    while (list->head) {
        struct write_buffer * tmp = list->head;
        for ( ; ; ) {
            int sz = write(s->fd, tmp->ptr, tmp->sz);
            if (sz < 0) {
                switch(errno) {
                    case EINTR:
                        continue;
                    case EAGAIN:
                        return -1;
                }
                force_close(s, result);
                return SOCKET_CLOSE;
            }
            s->wb_size -= sz;
            if (sz != tmp->sz) {
                tmp->ptr += sz;
                tmp->sz -= sz;
                return -1;
            }
            break;
        }
        list->head = tmp->next;
        free(tmp->buffer);
        free(tmp);
    }
    list->tail = nullptr;
    return -1;
}

int socket_server::send_list(struct socket *s, struct wb_list *list, struct socket_message *result)
{
    if (s->type == PROTOCOL_TCP) {
        return send_list_tcp(s, list, result);
    } else {
        return 0;
    }
}

int socket_server::list_uncomplete(struct wb_list * s)
{
    struct write_buffer * wb = s->head;
    if (wb == nullptr) {
        return 0;
    }
    return (void *)wb->ptr != wb->buffer;
}

void socket_server::raise_uncomplete(struct socket * s) 
{
    struct wb_list * low = &s->low;
    struct write_buffer * tmp = low->head;
    low->head = tmp->next;
    if (low->head == nullptr) {
        low->tail = nullptr;
    }

    struct wb_list * high = &s->high;
    assert(high->head == nullptr);

    tmp->next = nullptr;
    high->head = high->tail = tmp;
}

int socket_server::send_buffer(struct socket * s, struct socket_message * result)
{
    assert(!list_uncomplete(&s->low));
    if (send_list(s, &s->high, result) == SOCKET_CLOSE) {
        return SOCKET_CLOSE;
    }
    if (s->high.head == nullptr) {
        if (s->low.head != nullptr) {
            if (send_list(s, &s->low, result) == SOCKET_CLOSE) {
                return SOCKET_CLOSE;
            }
            if (list_uncomplete(&s->low)) {
                raise_uncomplete(s);
            }
        } else {
            event_fd.write(s->fd, s, false);
            if (s->type == SOCKET_TYPE_HALFCLOSE) {
                force_close(s, result);
                return SOCKET_CLOSE;
            }
        }
    }

    return -1;
}

int socket_server::close_socket(struct request_close * request, struct socket_message * result)
{
    int id = request->id;
    struct socket * s = &slot[HASH_ID(id)];
    if (s->type == SOCKET_TYPE_INVALID || s->id != id) {
        result->id = id;
        result->opaque = request->opaque;
        result->ud = 0;
        result->data = nullptr;
        return SOCKET_CLOSE;
    }
    if (!send_buffer_empty(s)) {
        int type = send_buffer(s, result);
        if (type == -1) {
            return type;
        }
    }
    if (send_buffer_empty(s)) {
        force_close(s, result);
        result->id = id;
        result->opaque = request->opaque;
        return SOCKET_CLOSE;
    }
    s->type = SOCKET_TYPE_HALFCLOSE;
    return -1;
}

void socket_server::setopt_socket(struct request_setopt * request)
{
    int id = request->id;
    struct socket * s = &slot[HASH_ID(id)];
    if (s->type == SOCKET_TYPE_INVALID || s->id != id) {
        return;
    }
    int val = request->value;
    setsockopt(s->fd, IPPROTO_TCP, request->what, &val, sizeof(val));
    return;
}

void socket_server::clear_closed_event(struct socket_message * result, int type)
{
    if (type == SOCKET_CLOSE || type == SOCKET_ERROR) {
        int id = result->id;
        for (int i = event_index; i < event_n; i++) {
            struct event * e = &ev[i];
            struct socket * s = (struct socket *)e->s;
            if (s) {
                if (s->type == SOCKET_TYPE_INVALID && s->id == id) {
                    e->s = nullptr;
                }
            }
        }
    }
}

int socket_server::forward_message_tcp(struct socket * s, struct socket_message * result)
{
    int sz = s->size;
    char * buffer = (char *)malloc(sz);
    int n = (int)read(s->fd, buffer, sz);
    if (n < 0) {
        free(buffer);
        switch(errno) {
            case EINTR:
                break;
            case EAGAIN:
                fprintf(stderr, "socket-server: EAGAIN capture.\n");
                break;
            default:
                force_close(s, result);
        }
        return -1;
    }
    if (n == 0) {
        free(buffer);
        return -1;
    }
    if (s->type == SOCKET_TYPE_HALFCLOSE) {
        free(buffer);
        return -1;
    }
    if (n == sz) {
        s->size *= 2;
    } else if (sz > MIN_READ_BUFFER && n*2 < sz) {
        s->size /= 2;
    }

    result->opaque = s->opaque;
    result->id = s->id;
    result->ud = n;
    result->data = buffer;
    return SOCKET_DATA;
}

void socket_server::server_exit()
{
    struct request_package request;
    send_request(&request, 'X', 0);
}

void socket_server::server_close(uintptr_t opaque, int id) 
{
    struct request_package request;
    request.u.close.id = id;
    request.u.close.opaque = opaque;
    send_request(&request, 'K', sizeof(request.u.close));
}

int64_t socket_server::server_send(int id, const void * buffer, int sz)
{
    struct socket * s = &slot[HASH_ID(id)];
    if (id != s->id || s->type == SOCKET_TYPE_INVALID) {
        //free(buffer);
        return -1;
    }

    struct request_package request;
    request.u.send.id = id;
    request.u.send.sz = sz;
    request.u.send.buffer = (char *)buffer;

    send_request(&request, 'D', sizeof(request.u.send));

    return s->wb_size;
}

int socket_server::send_socket(struct request_send * request, struct socket_message * result, int priority)
{
    int id = request->id;
    struct socket *s = &slot[HASH_ID(id)];
    if (s->type == SOCKET_TYPE_INVALID
            || s->id != id
            || s->type == SOCKET_TYPE_HALFCLOSE
            || s->type == SOCKET_TYPE_PACCEPT) {
        return -1;
    }
    assert(s->type != SOCKET_TYPE_PLISTEN && s->type != SOCKET_TYPE_LISTEN);
    if (send_buffer_empty(s) && s->type == SOCKET_TYPE_CONNECTED) {
        if (s->protocol == PROTOCOL_TCP) {
            int n = write(s->fd, request->buffer, request->sz);
            if (n < 0) {
                switch (errno) {
                    case EINTR:
                    case EAGAIN:
                        n = 0;
                        break;
                    default:
                        fprintf(stderr, "socket-server: write to %d (fd=%d) error: %s.\n", id, s->fd, strerror(errno));
                        force_close(s, result);
                        return SOCKET_CLOSE;
                }
            }
            if (n == request->sz)
            {
                return -1;
            }
            event_fd.write(s->fd, s, true);
        }
    }

    return -1;
}

