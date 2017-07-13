#ifndef __CUBE_EVENT_LOOP_H__
#define __CUBE_EVENT_LOOP_H__

#include <functional>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>

namespace cube {

#define CUBE_OK 0
#define CUBE_ERR -1

typedef uint64_t TimerId;

class Eventor;
class Poller;
class TimerQueue;

class EventLoop {
    public:
        // 回调函数，用户可以将自己的逻辑注册到事件循环中
        typedef std::function<void()> Task;

        EventLoop();
        ~EventLoop();

        static EventLoop *Current();
        // thread-safety
        void Post(const Task &task);

        void Loop();
        int LoopOnce(int poll_timeout_ms);

        void Stop();

        // wrap for poller
        void UpdateEvents(Eventor *e);
        void RemoveEvents(Eventor *e);

        // wrap for timer queue
        TimerId RunAt(const Task &task, int64_t expiration_ms);
        TimerId RunAfter(const Task &task, int64_t delay_ms);
        TimerId RunPeriodic(const Task &task, int64_t interval_ms);
        TimerId RunPeriodic(const Task &task, int64_t expiration_ms, int64_t interval_ms);
        void CancelTimer(TimerId time_id);

        bool IsLoopThread() const { return m_thread_id == std::this_thread::get_id(); }
        void AssertInLoopThread() const {
            if (!IsLoopThread()) {
                abort();
            }
        }

        void WakeUp();

    private:
        void HandleEvents(int revents);

    private:
        // thread id
        // 事件轮询线程的线程ID
        std::thread::id m_thread_id;

        // wake up
        // linux event fd，用于唤醒事件循环
        int m_wakeup_fd;
        std::unique_ptr<Eventor> m_wakeup_eventor;

        // poller
        // poller是对linux epoll的封装，用户管理事件
        std::unique_ptr<Poller> m_poller;

        // timer queue
        // 定时任务队列，用于执行定时任务
        std::unique_ptr<TimerQueue> m_timer_queue;

        // task
        // Task实际是回调函数，用户的执行逻辑
        std::mutex m_tasks_mutex;
        std::vector<Task> m_tasks;

        // running flag
        bool m_running;
};

}

#endif
