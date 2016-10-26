#ifndef __BLOCK_QUEUE_H__
#define __BLOCK_QUEUE_H__

#include <queue>
#include <mutex>
#include <condition_variable>

namespace cube {

template<typename T>
class BlockQueue {
    public:
        void Push(const T &task);
        T Pop();

    private:
        std::queue<T> m_queue;
        std::mutex m_queue_mutex;
        std::condition_variable m_queue_cv;

    private:
        BlockQueue(const BlockQueue &) = delete;
        BlockQueue(BlockQueue &&) = delete;
        BlockQueue &operator=(const BlockQueue &) = delete;
        BlockQueue &operator=(BlockQueue &&) = delete;
};

template<typename T>
void BlockQueue<T>::Push(const T &task) {
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    bool empty = m_queue.empty();
    m_queue.push(task);
    if (empty)
        m_queue_cv.notify_all();
}

template<typename T>
T BlockQueue<T>::Pop() {
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    while (m_queue.empty()) m_queue_cv.wait(lock);
    T task = m_queue.front();
    m_queue.pop();
    return task;
}

}

#endif
