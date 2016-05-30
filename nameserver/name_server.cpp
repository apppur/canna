#include <iostream>
#include <sstream>
#include <iomanip>
#include "canna_core.h"
#include "name_server.h"
#include "name_ctrl.h"

name_server::name_server()
{
}

name_server::~name_server()
{
    delete m_responder;
    delete m_publisher;
    delete m_context;
}

int name_server::initialize()
{
    m_context = new zmq::context_t(1);
    m_responder = nullptr;
    m_publisher = nullptr;

    return 0;
}

zmq::socket_t * name_server::sock_create(int type)
{
    return new zmq::socket_t(*m_context, type);
}

void name_server::sock_bind(zmq::socket_t * sock, const char * addr)
{
    if (sock) {
        sock->bind(addr);
    }
}

void name_server::sock_setopt(zmq::socket_t * sock)
{
    return;
}

void name_server::sock_connect(zmq::socket_t * sock, const char * addr)
{
    if (sock) {
        sock->connect(addr);
    }
}

std::string name_server::sock_recv(zmq::socket_t &socket)
{
    zmq::message_t message;
    socket.recv(&message);

    return std::string(static_cast<char*>(message.data()), message.size());
}

bool name_server::sock_send(zmq::socket_t &socket, const std::string &string)
{
    zmq::message_t message(string.size());
    memcpy(message.data(), string.data(), string.size());

    bool result = socket.send(message);

    return result;
}

bool name_server::sock_sendmore(zmq::socket_t &socket, const std::string &string)
{
    zmq::message_t message(string.size());
    memcpy(message.data(), string.data(), string.size());

    bool result = socket.send(message, ZMQ_SNDMORE);

    return result;
}

void name_server::sock_dump(zmq::socket_t &socket)
{
    std::cout << "--------------------------------------------------" << std::endl;
    while (true) {
        zmq::message_t message;
        socket.recv(&message);

        int size = message.size();
        std::string data(static_cast<char *>(message.data()), size);

        bool is_text = true;
        unsigned int count;
        unsigned char c;
        for (count = 0; count < size; count++) {
            c = data[count];
            if (c < 32 || c > 127) {
                is_text = false;
            }
        }
        std::cout << "[" << std::setfill('0') << std::setw(3) << size << "]";
        for (count = 0; count < size; count++) {
            if (is_text) {
                std::cout << (char)data[count];
            } else {
                std::cout << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)data[count];
            }
        }
        std::cout << std::endl;

        int more = 0;
        size_t more_size = sizeof(more);
        socket.getsockopt (ZMQ_RCVMORE, &more, &more_size);
        
        if (!more) {
            break;
        }
    }
}

std::string name_server::sock_setid(zmq::socket_t &socket)
{
    std::stringstream ss;
    ss << std::hex << std::uppercase
        << std::setw(4) << std::setfill('0') << CANNA_RAND(0x10000) << "-"
        << std::setw(4) << std::setfill('0') << CANNA_RAND(0x10000);
    socket.setsockopt(ZMQ_IDENTITY, ss.str().c_str(), ss.str().length());

    return ss.str();
}

void name_server::responder_create()
{
    m_responder = sock_create(ZMQ_REP);
    //m_responder = (zmq::socket_t *) new zmq::socket_t (*m_context, ZMQ_REP);
}

void name_server::publisher_create()
{
    m_publisher = sock_create(ZMQ_PUB);
}

void name_server::responder_bind(const char * addr)
{
    sock_bind(m_responder, addr);
}

void name_server::publisher_bind(const char * addr)
{
    sock_bind(m_publisher, addr);
}

void name_server::server_loop()
{
    zmq::pollitem_t pollset[] = {
        {(void *)*m_responder, 0, ZMQ_POLLIN, 0}
    };

    name_ctrl namectrl;

    while (true) {
        canna_sleep(100);
        zmq::poll(pollset, 1, 0);

        if (pollset[0].revents & ZMQ_POLLIN) {

            std::string group = sock_recv(*m_responder);

            std::string name  = namectrl.allotnamepair(group);
            std::cout << "GET REQUEST: " << group << std::endl;
    
            sock_send(*m_responder, name);
            sock_send(*m_publisher, name);
        }
    }

    return;
}
