#include <iostream>
#include <sstream>
#include <ipmanip>
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


