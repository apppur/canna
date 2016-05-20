#ifndef _SOCKET_EPOLL_H
#define _SOCKET_EPOLL_H

#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

class socket_poll
{
    public:
        bool invalid() { return m_fd == -1; }
        void create() { m_fd = epoll_create(1024); }
        void release() { close(m_fd); }
        
        int add(int sock, void *ud) {
            struct epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.ptr = ud;
            if (epoll_ctl(m_fd, EPOLL_CTL_ADD, sock, &ev) == -1) {
                return 1;
            }
            return 0;
        }
        void del(int sock) {
            epoll_ctl(m_fd, EPOLL_CTL_DEL, sock, nullptr);
        }

        void write(int sock, void *ud, bool enable) {
            struct epoll_event ev;
            ev.events = EPOLLIN | (enable ? EPOLLOUT : 0);
            ev.data.ptr = ud;
            epoll_ctl(m_fd, EPOLL_CTL_MOD, sock, &ev);
        }

        int wait(struct event *e, int max) {
            struct epoll_event ev[max];
            int n = epoll_wait(m_fd, ev, max, -1);
            for (int i = 0; i < n; i++) {
                e[i].s = ev[i].data.ptr;
                unsigned flag = ev[i].events;
                e[i].write = (flag & EPOLLOUT) != 0;
                e[i].read = (flag & EPOLLIN) != 0;
            }

            return n;
        }

        void nonblocking() {
            int flag = fcntl(m_fd, F_GETFL, 0);
            if (-1 == flag) {
                return;
            }

            fcntl(m_fd, F_SETFL, flag | O_NONBLOCK);
        }
    private:
        int m_fd;
};

#endif
