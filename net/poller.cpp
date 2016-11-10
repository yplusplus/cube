#include <string.h>
#include <assert.h>
#include <unistd.h> // ::close

#include "poller.h"
#include "event_loop.h"
#include "eventor.h"

namespace cube {

Poller::Poller(EventLoop *event_loop) 
    : m_event_loop(event_loop),
    m_epoll_fd(::epoll_create(EPOLL_EVENT_SIZE)) {
        assert(m_epoll_fd >= 0);
}

Poller::~Poller() {
    ::close(m_epoll_fd);
}

bool Poller::UpdateEvents(Eventor *eventor) {
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
    m_event_loop->AssertInLoopThread();
    assert(m_eventors.count(eventor->Fd()) > 0);
    m_eventors.erase(eventor->Fd());
    return EpollOperate(EPOLL_CTL_DEL, eventor);
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
