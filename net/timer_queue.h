#ifndef __CUBE_TIMER_QUEUE_H__
#define __CUBE_TIMER_QUEUE_H__

#include <set>
#include <map>
#include <cstdint>
#include <functional>

namespace cube {

typedef uint64_t TimerId;
typedef std::function<void()> TimerTask;

// 定时任务
// 成员包括任务ID，任务回调函数（执行逻辑），到期时间，循环执行的时间间隔
// expiration_ms 表示当前时间超过expiration_ms后，该任务可以被执行，并从队列中删除
// interval_ms表示，超时后，需要重新添加到任务队列，在在interval_ms后再次执行
// 若interval_ms=-1, 表示不需要被循环执行
struct Timer {
    TimerId timer_id;
    TimerTask task;
    int64_t expiration_ms;
    int64_t interval_ms;

    // 重载了<操作符号
    // Timer要插入std::set中，所以实现比较的逻辑
    bool operator<(const Timer &rhs) const {
        if (expiration_ms != rhs.expiration_ms)
            return expiration_ms < rhs.expiration_ms;
        return timer_id < rhs.timer_id;
    }
};

// 定时任务队列
// 可以往队列中注册定时任务，当时间到达时，从队列中取出并执行。
class TimerQueue {
    public:
        TimerId AddTimer(const TimerTask &task, int64_t expiration_ms, int64_t interval_ms);
        void RemoveTimer(TimerId timer_id);

        // next_expiration == -1 when no timers
        int Expire(int64_t now_ms, int64_t &next_expiration);

    private:
        static TimerId NextTimerId();

        // 无符号64位整型来作为任务ID
        // 每往任务队列中添加一个任务，该ID自增（线程安全），并作为新任务的ID
        static TimerId m_next_timer_id;  // atomic

        void AddTimer(const Timer &timer);

    private:
        std::set<Timer> m_timers;
        std::map<TimerId, int64_t> m_timer_expiration;
};

}

#endif
