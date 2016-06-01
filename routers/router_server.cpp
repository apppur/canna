#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>

#include "router_server.h"
#include "nameinfo.pb.h"

router_server::router_server()
{
}

router_server::~router_server()
{
    delete m_clusterfe;
    delete m_clusterbe;
    delete m_subscribe;
    delete m_requester;
    delete m_context;
}

int router_server::initialize()
{
    m_context = new zmq::context_t(1);
    m_clusterfe = nullptr;
    m_clusterbe = nullptr;
    m_subscribe = nullptr;
    m_requester = nullptr;
    return 0;
}

zmq::socket_t * router_server::sock_create(int type)
{
    return new zmq::socket_t(*m_context, type);
}

void router_server::sock_bind(zmq::socket_t * sock, const char * addr)
{
    if (sock) {
        sock->bind(addr);
    }
}

void router_server::sock_identity(zmq::socket_t * sock, const char * addr)
{
    if (sock) {
        sock->setsockopt(ZMQ_IDENTITY, addr, strlen(addr));
    }
}

void router_server::sock_setfilter(zmq::socket_t * sock, const char * filter)
{
    if (sock) {
        sock->setsockopt(ZMQ_SUBSCRIBE, filter, strlen(filter));
    }
}

void router_server::sock_connect(zmq::socket_t * sock, const char * addr)
{
    if (sock) {
        sock->connect(addr);
    }
}

std::string router_server::sock_recv(zmq::socket_t &socket)
{
    zmq::message_t message;
    socket.recv(&message);

    return std::string(static_cast<char*>(message.data()), message.size());
}

bool router_server::sock_send(zmq::socket_t &socket, const std::string &string)
{
    zmq::message_t message(string.size());
    memcpy(message.data(), string.data(), string.size());

    bool result = socket.send(message);

    return result;
}

bool router_server::sock_sendmore(zmq::socket_t &socket, const std::string &string)
{
    zmq::message_t message(string.size());
    memcpy(message.data(), string.data(), string.size());

    bool result = socket.send(message, ZMQ_SNDMORE);

    return result;
}

void router_server::router_init() 
{
    m_requester = sock_create(ZMQ_REQ);
    sock_connect(m_requester, "tcp://localhost:5555");
    m_subscribe = sock_create(ZMQ_SUB);
    sock_connect(m_subscribe, "tcp://localhost:5556");
    sock_setfilter(m_subscribe, "");
}

std::vector<std::string> router_server::router_name()
{
    sock_send(*m_requester, "cluster");
    std::string res = sock_recv(*m_requester);

    NameInfo nameinfo;
    nameinfo.ParseFromString(res);
    std::vector<std::string> namelist;
    for (int i = 0; i < nameinfo.name_size(); i++) {
        namelist.push_back(nameinfo.name(i));
    }

    return namelist;
}
