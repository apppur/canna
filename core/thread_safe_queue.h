#ifndef _THREAD_SAFE_QUEUE_H
#define _THREAD_SAFE_QUEUE_H

#include <utility>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

template<typename T>
class ThreadSafeQueue {
    public:
        ThreadSafeQueue() {}

        void push(T value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(value));
            m_cond.notify_one();
        }

        void wait_and_pop(T& value) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.wait(lock, [this]{return !m_queue.empty();});
            value = std::move(m_queue.front());
            m_queue.pop();
        }

        std::shared_ptr<T> wait_and_pop() {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.wait(lock, [this]{return !m_queue.empty();});
            std::shared_ptr<T> res(std::make_shared<T>(std::move(m_queue.front())));
            m_queue.pop();
            return res;
        }

        bool try_pop(T& value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty())
                return false;
            value = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }

        std::shared_ptr<T> try_pop() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty())
                return std::shared_ptr<T>();
            std::shared_ptr<T> res(std::make_shared<T>(std::move(m_queue.front())));
            m_queue.pop();
            return res;
        }

        bool empty() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.empyt();
        }

    private:
        mutable std::mutex m_mutex;
        std::queue<T> m_queue;
        std::condition_variable m_cond;
};

#endif
