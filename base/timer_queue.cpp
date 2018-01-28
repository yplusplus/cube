#include <assert.h>

#include "timer_queue.h"

namespace cube {

TimerId TimerQueue::m_next_timer_id(1);

TimerId TimerQueue::NextTimerId() {
    // 线程安全地自增
    TimerId seq = __sync_fetch_and_add(&m_next_timer_id, 1);
    return seq;
}

TimerId TimerQueue::AddTimer(const TimerTask &task, int64_t expiration_ms, int64_t interval_ms) {
    // 往任务队列中添加任务
    // 先获取新的任务ID
    // 构造定时任务: 任务ID，回调函数（执行逻辑），过期时间，执行的时间间隔
    // 注册
    TimerId timer_id = NextTimerId();
    Timer timer = { timer_id, task, expiration_ms, interval_ms};
    AddTimer(timer);
    return timer_id;
}

void TimerQueue::AddTimer(const Timer &timer) {
    // 保存定时任务到std::set中
    // 同时将任务ID和超时时间进行映射，方便查询
    auto ret = m_timers.insert(timer);
    assert(ret.second);
    m_timer_expiration[timer.timer_id] = timer.expiration_ms;
}

void TimerQueue::RemoveTimer(TimerId timer_id) {
    auto it = m_timer_expiration.find(timer_id);
    if (it == m_timer_expiration.end())
        return;
    struct Timer timer;
    timer.timer_id = timer_id;
    timer.expiration_ms = it->second;
    // 从set，map中删除
    m_timers.erase(timer);
    m_timer_expiration.erase(it);
}

int TimerQueue::Expire(int64_t now_ms, int64_t &next_expiration) {

    int result = 0;
    // 遍历set中保存的定时任务
    // 由于定时任务重载了<运算符，所以遍历时，按到期时间从小到大输出
    while (!m_timers.empty()) {
        auto it = m_timers.begin();
        if (it->expiration_ms > now_ms) break;

        // 从任务队列中删除该任务
        Timer timer = *it;
        RemoveTimer(timer.timer_id);

        // re-add timer
        // 如果设置了时间间隔，则该任务需要被循环执行，重新添加到任务队列中
        if (timer.interval_ms > 0) {
            timer.expiration_ms += timer.interval_ms;
            AddTimer(timer);
        }

        // 已到达超时时间的任务，执行
        timer.task();
        // 统计本轮唤醒中，有多少超时任务
        result++;
    }

    // 设置下一次的唤醒时间
    // 如果队列已经为空，则不必要再次唤醒，设置为空
    // 否则，设置为超时时间最早的那个任务
    if (m_timers.empty())
        next_expiration = -1;
    else
        next_expiration = m_timers.begin()->expiration_ms;

    return result;
}

}
