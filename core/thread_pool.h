#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <atomic>
#include <thread>
#include <functional>
#include <vector>
#include <iostream>
#include "thread_safe_queue.h"

class ThreadJoin {
    private:
        std::vector<std::thread> & m_threads;
    public:
        explicit ThreadJoin(std::vector<std::thread>& threads) :
            m_threads(threads)
    {}
        ~ThreadJoin() {
            for (unsigned long i = 0; i < m_threads.size(); i++) {
                if (m_threads[i].joinable())
                    m_threads[i].join();
            }
        }
};

class ThreadPool {
    private:
        std::atomic_bool m_done;
        ThreadSafeQueue<std::function<void()>> m_queue;
        std::vector<std::thread> m_threads;
        ThreadJoin m_joiner;

        void work() {
            while (!m_done) {
                //std::cout << "work thread run ..." << std::endl;
                std::function<void()> task;
                if (m_queue.try_pop(task)) {
                    task();
                } else {
                    std::this_thread::yield();
                }
            }
        }

    public:
        ThreadPool() : m_done(false), m_joiner(m_threads) {
            unsigned thread_count = std::thread::hardware_concurrency();
            thread_count = thread_count < 2 ? 2 : thread_count;
            try {
                for (unsigned i = 0; i < thread_count; i++) {
                    std::cout << "work " << i  << " thread run ..." << std::endl;
                    m_threads.push_back(std::thread(&ThreadPool::work, this));
                }
            } catch (...) {
                m_done = true;
                throw;
            }
        }

        ~ThreadPool() {
            m_done = true;
        }

        template<typename Function>
        void submit(Function fun) {
            m_queue.push(std::function<void()>(fun));
        }
};

#endif
