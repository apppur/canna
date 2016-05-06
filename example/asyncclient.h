#ifndef _ASYNC_CLIENT_H
#define _ASYNC_CLIENT_H
#include <stdio.h>
#include <string.h>
#include <exception>
#include "canna_core.h"
#include "zmq.hpp"

class asyncclient
{
    public:
        asyncclient() : m_context(1), m_socket(m_context, ZMQ_DEALER) {}
        ~asyncclient() {}

        void start()
        {
            char identity[10] = {};
            sprintf(identity, "%04X-%04X", CANNA_RAND(0x10000), CANNA_RAND(0x10000));
            printf("client identity: %s\n", identity);
            m_socket.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
            m_socket.connect("tcp://localhost:5570");
            zmq::pollitem_t items[] = { (void *)m_socket, 0, ZMQ_POLLIN, 0 };
            int count = 0;

            try {
                while (true) {
                    for (int i = 0; i < 100; i++) {
                        zmq::poll(items, 1, 10);
                        if (items[0].revents & ZMQ_POLLIN) {
                            printf("receive: %s\n", identity);
                            canna_dump(m_socket);
                        }
                    }

                    char request[16] = {};
                    sprintf(request, "request #%d", ++count);
                    m_socket.send(request, strlen(request));
                }
            }
            catch (std::exception &e) {}
        }

    private:
        zmq::context_t  m_context;
        zmq::socket_t   m_socket;
};
#endif
