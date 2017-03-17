#include <sys/eventfd.h>
#include <unistd.h>
#include <assert.h>

#include "event_loop.h"
#include "eventor.h"
#include "poller.h"
#include "timer_queue.h"
#include "time_util.h"
#include "base/log.h"

using namespace std::placeholders;

namespace cube {

static int CreateEventFd() {
    int event_fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (event_fd < 0) {
        abort();
    }
    return event_fd;
}

EventLoop::EventLoop() 
    : m_thread_id(std::this_thread::get_id()),
    m_wakeup_fd(CreateEventFd()),
    m_wakeup_eventor(new Eventor(this, m_wakeup_fd)),
    m_poller(new Poller(this)),
    m_timer_queue(new TimerQueue),
    m_running(false) {
    m_wakeup_eventor->SetEventsCallback(std::bind(&EventLoop::HandleEvents, this, _1));
    m_wakeup_eventor->EnableReading();
}

EventLoop::~EventLoop() {
    ::close(m_wakeup_fd);
}

__thread EventLoop *tls_ptr = NULL;
EventLoop *EventLoop::Current() {
    if (tls_ptr == NULL)
        tls_ptr = new EventLoop;
    return tls_ptr;
}

void EventLoop::Post(const Task &task) {
    bool is_empty = false;
    {
        std::unique_lock<std::mutex> lock(m_tasks_mutex);
        is_empty = m_tasks.empty();
        m_tasks.push_back(task);
    }

    // Wake up to run task when this is the first task
    // and not in loop thread
    if (is_empty && !IsLoopThread()) {
        WakeUp();
    }
}

void EventLoop::UpdateEvents(Eventor *eventor) {
    m_poller->UpdateEvents(eventor);
}

void EventLoop::RemoveEvents(Eventor *eventor) {
    m_poller->RemoveEvents(eventor);
}

TimerId EventLoop::RunAt(const Task &task, int64_t expiration_ms) {
    return m_timer_queue->AddTimer(task, expiration_ms, 0);
}

TimerId EventLoop::RunAfter(const Task &task, int64_t delay_ms) {
    return m_timer_queue->AddTimer(task, TimeUtil::CurrentTimeMillis() + delay_ms, 0);
}

TimerId EventLoop::RunPeriodic(const Task &task, int64_t interval_ms) {
    return m_timer_queue->AddTimer(task, TimeUtil::CurrentTimeMillis() + interval_ms, interval_ms);
}

TimerId EventLoop::RunPeriodic(const Task &task, int64_t expiration_ms, int64_t interval_ms) {
    return m_timer_queue->AddTimer(task, expiration_ms, interval_ms);
}

void EventLoop::CancelTimer(TimerId timer_id) {
    m_timer_queue->RemoveTimer(timer_id);
}

void EventLoop::Loop() {
    AssertInLoopThread();
    m_running = true;
    while (m_running) {
        // wait for 5ms at most
        LoopOnce(5);
    }
}

int EventLoop::LoopOnce(int poll_timeout_ms) {
    AssertInLoopThread();

    int result = 0;

    int64_t now_ms = TimeUtil::CurrentTimeMillis();
    std::vector<Task> tasks;
    {
        std::unique_lock<std::mutex> lock(m_tasks_mutex);
        tasks.swap(m_tasks);
    }

    for (auto it = tasks.begin(); it != tasks.end(); it++) {
        (*it)();
    }
    result += tasks.size();

    // timer queue
    int64_t next_expiration = -1;
    result += m_timer_queue->Expire(now_ms, next_expiration);
    
    // poller
    if (next_expiration > now_ms 
            && (poll_timeout_ms == -1 || (next_expiration - now_ms) < poll_timeout_ms)) {
        poll_timeout_ms = next_expiration - now_ms;
    }

    {
        // wake up immediately when there are pending tasks
        std::unique_lock<std::mutex> lock(m_tasks_mutex);
        if (!m_tasks.empty()) poll_timeout_ms = 0;
    }

    std::vector<Eventor *> active_eventors;
    m_poller->Poll(poll_timeout_ms, active_eventors);
    for (auto it = active_eventors.begin();
            it != active_eventors.end(); it++) {
        (*it)->HandleEvents();
        result++;
    }
    return result;
}

void EventLoop::Stop() {
    m_running = false;
    if (!IsLoopThread())
        WakeUp();
}

void EventLoop::HandleEvents(int revents) {
    uint64_t dummy = 0;
    ::read(m_wakeup_fd, &dummy, sizeof(dummy));
}

void EventLoop::WakeUp() {
    uint64_t one = 1;
    ::write(m_wakeup_fd, &one, sizeof(one));
}

}

