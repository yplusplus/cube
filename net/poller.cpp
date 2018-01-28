#include <string.h>
#include <assert.h>
#include <unistd.h> // ::close

#include "poller.h"
#include "event_loop.h"
#include "eventor.h"

namespace cube {

namespace net {

Poller::Poller(EventLoop *event_loop)
    : m_event_loop(event_loop),
    m_epoll_fd(::epoll_create(EPOLL_EVENT_SIZE)) {
        assert(m_epoll_fd >= 0);
}

// 析构函数，关闭epoll句柄
Poller::~Poller() {
    ::close(m_epoll_fd);
}

bool Poller::UpdateEvents(Eventor *eventor) {
    // EPOLL_CTL_ADD 和 EPOLL_CTL_MOD 统一入口
    // 若poller中还没有注册该fd，则ADD该fd，否则MOD该fd上的事件
    m_event_loop->AssertInLoopThread();
    int operation = 0;
    if (m_eventors.count(eventor->Fd())) {
        operation = EPOLL_CTL_MOD;
    } else {
        operation = EPOLL_CTL_ADD;
        m_eventors[eventor->Fd()] = eventor;
    }
    return EpollOperate(operation, eventor);
}

bool Poller::RemoveEvents(Eventor *eventor) {
    // 从poller中删除一个fd，assert保证poller中已经注册了该fd
    m_event_loop->AssertInLoopThread();

    auto it = m_eventors.find(eventor->Fd());
    if (it != m_eventors.end()) {
        m_eventors.erase(it);
        return EpollOperate(EPOLL_CTL_DEL, eventor);
    }
    return true;
}

bool Poller::EpollOperate(int operation, Eventor *eventor) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = eventor->Events();
    ev.data.ptr = eventor;
    if (::epoll_ctl(m_epoll_fd, operation, eventor->Fd(), &ev) < 0)
        return false;
    return true;
}

void Poller::Poll(int timeout_ms, std::vector<Eventor *> &eventors) {
    // 事件轮询，对epoll_wait的封装
    // 每次唤醒的Eventor保存到用户提供的eventors数组中（vector）
    eventors.clear();
    int num_events = ::epoll_wait(m_epoll_fd, m_epoll_events, EPOLL_EVENT_SIZE, timeout_ms);
    if (num_events < 0) {
        if (errno != EINTR) {
            // error
        } else {
            num_events = 0;
        }
    } else if (num_events > 0) {
        eventors.resize(num_events);
        for (int i = 0; i < num_events; i++) {
            eventors[i] = static_cast<Eventor *>(m_epoll_events[i].data.ptr);
            eventors[i]->SetRevents(m_epoll_events[i].events);
        }
    }
}

}

}
