#include <assert.h>

#include "timer_queue.h"

namespace cube {

TimerId TimerQueue::m_next_timer_id(1);

TimerId TimerQueue::NextTimerId() {
    TimerId seq = __sync_fetch_and_add(&m_next_timer_id, 1);
    return seq;
}

TimerId TimerQueue::AddTimer(const TimerTask &task, int64_t expiration_ms, int64_t interval_ms) {
    TimerId timer_id = NextTimerId();
    Timer timer = { timer_id, task, expiration_ms, interval_ms};
    AddTimer(timer);
    return timer_id;
}

void TimerQueue::AddTimer(const Timer &timer) {
    auto ret = m_timers.insert(timer);
    assert(ret.second);
}

void TimerQueue::RemoveTimer(TimerId timer_id) {
    m_deleted_timers.insert(timer_id);
}

int TimerQueue::Expire(int64_t now_ms, int64_t &next_expiration) {

    int result = 0;
    while (!m_timers.empty()) {
        auto it = m_timers.begin();
        if (it->expiration_ms > now_ms) break;

        Timer timer = *it;
        m_timers.erase(it);
        if (m_deleted_timers.count(timer.timer_id)) {
            m_deleted_timers.erase(timer.timer_id);
            continue;
        }

        // re-add timer
        if (timer.interval_ms > 0) {
            timer.expiration_ms += timer.interval_ms;
            AddTimer(timer);
        }

        timer.task();
        result++;
    }

    if (m_timers.empty()) 
        next_expiration = -1;
    else 
        next_expiration = m_timers.begin()->expiration_ms;

    return result;
}

}
