#ifndef _ASYNC_SERVER_H
#define _ASYNC_SERVER_H
#include <exception>
#include <vector>
#include <thread>
#include <functional>

#include "zmq.hpp"
#include "asyncworker.h"

class asyncserver
{
    public:
        asyncserver() : m_context(1), m_frontend(m_context, ZMQ_ROUTER), m_backend(m_context, ZMQ_DEALER) {}
        ~asyncserver() {}

        enum { THREAD_MAX = 5 };

        void run() {
            m_frontend.bind("tcp://*:5570");
            m_backend.bind("tcp://*:5571");
            std::vector<asyncworker *> worker;
            std::vector<std::thread *> pool;
            for (int i = 0; i < THREAD_MAX; i++) {
                worker.push_back(new asyncworker(m_context, ZMQ_DEALER));
                pool.push_back(new std::thread(std::bind(&asyncworker::work, worker[i])));
                pool[i]->detach();
            }

            try {
                zmq::proxy(&m_frontend, &m_backend, nullptr);
            } catch (std::exception &e) {
            }

            for (int i = 0; i < THREAD_MAX; i++) {
                delete worker[i];
                delete pool[i];
            }
        }
    private:
        zmq::context_t  m_context;
        zmq::socket_t   m_frontend;
        zmq::socket_t   m_backend;
};

#endif
