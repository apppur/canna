#ifndef _NAME_SERVER_H
#define _NAME_SERVER_H

#include <string>

#include "zmq.hpp"

class name_server
{
    public:
        name_server();
        ~name_server();
        int initialize();

    public:
        zmq::socket_t * sock_create(int type);
        void sock_bind(zmq::socket_t * sock, const char * addr);
        void sock_setopt(zmq::socket_t * sock);
        void sock_connect(zmq::socket_t * sock, const char * addr);

        std::string sock_recv(zmq::socket_t &socket);
        bool sock_send(zmq::socket_t &socket, const std::string &string);
        bool sock_sendmore(zmq::socket_t &socket, const std::string &string);
        void sock_dump(zmq::socket_t &socket);
        std::string sock_setid(zmq::socket_t &socket);

        zmq::context_t * get_context() { return m_context; }
        zmq::socket_t * get_responder() { return m_responder; }
        zmq::socket_t * get_publisher() { return m_publisher; }
    private:
        zmq::context_t * m_context;
        zmq::socket_t * m_responder;
        zmq::socket_t * m_publisher;
};

#endif
