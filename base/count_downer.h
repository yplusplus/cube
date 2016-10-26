#ifndef __COUNT_DOWN_LOCK_H__
#define __COUNT_DOWN_LOCK_H__

#include <mutex>
#include <condition_variable>

namespace cube {

class CountDowner {
    public:
        CountDowner(int count) : m_count(count) {}

        void Wait() {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (m_count > 0) {
                m_cv.wait(lock);
            }
        }

        void CountDown() {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_count--;
            if (m_count == 0) {
                m_cv.notify_all();
            }
        }

        inline int Count() {
            std::unique_lock<std::mutex> lock(m_mutex);
            return m_count;
        }

    private:
        int m_count;
        std::mutex m_count_mutex;
        std::condition_variable m_count_cv;
};

}

#endif

