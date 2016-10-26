#ifndef __CUBE_TIMER_QUEUE_H__
#define __CUBE_TIMER_QUEUE_H__

#include <set>
#include <cstdint>
#include <functional>

namespace cube {

typedef uint64_t TimerId;
typedef std::function<void()> TimerTask;

struct Timer {
    TimerId timer_id;
    TimerTask task;
    int64_t expiration_ms;   
    int64_t interval_ms;
    bool operator<(const Timer &rhs) const {
        return expiration_ms < rhs.expiration_ms;
    }
};

class TimerQueue {
    public:
        TimerId AddTimer(const TimerTask &task, int64_t expiration_ms, int64_t interval_ms);
        void RemoveTimer(TimerId timer_id);

        // next_expiration == -1 when no timers
        int Expire(int64_t now_ms, int64_t &next_expiration);

    private:
        static TimerId NextTimerId();

        static TimerId m_next_timer_id;  // atomic

        void AddTimer(const Timer &timer);

    private:
        std::set<Timer> m_timers;
        std::set<TimerId> m_deleted_timers;
};

}

#endif
