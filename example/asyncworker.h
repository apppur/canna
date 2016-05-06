#ifndef _ASYNC_WORKER_H
#define _ASYNC_WORKER_H
#include <exception>
#include "canna_core.h"
#include "zmq.hpp"

class asyncworker
{
    public:
        asyncworker(zmq::context_t &context, int type)
            : m_context(context), m_socket(m_context, type)
        {}
        ~asyncworker() {}

        void work() {
            m_socket.connect("tcp://localhost:5571");
            try {
                while (true) {
                    zmq::message_t identity;
                    zmq::message_t msg;
                    zmq::message_t copied_id;
                    zmq::message_t copied_msg;
                    m_socket.recv(&identity);
                    m_socket.recv(&msg);
                    int count = CANNA_RAND(5);
                    for (int i = 0; i < count; i++) {
                        canna_sleep(CANNA_RAND(1000) + 1);
                        copied_id.copy(&identity);
                        copied_msg.copy(&msg);
                        m_socket.send(copied_id, ZMQ_SNDMORE);
                        m_socket.send(copied_msg);
                    }
                }
            }
            catch (std::exception &e) {}
        }
    private:
        zmq::context_t  &m_context;
        zmq::socket_t   m_socket;
};

#endif
