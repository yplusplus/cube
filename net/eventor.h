#ifndef __CUBE_EVENTOR_H__
#define __CUBE_EVENTOR_H__

#include <memory>
#include <functional>

#include "callbacks.h"
#include "poller.h"

namespace cube {

namespace net {

class EventLoop;

class Eventor {
    public:
        Eventor(EventLoop *event_loop, int fd);
        ~Eventor();

        void SetEventsCallback(const EventsCallback &cb) { m_events_callback = cb; }

        void HandleEvents();

        void EnableReading() { m_events |= Poller::POLLIN; Update(); }
        void EnableWriting() { m_events |= Poller::POLLOUT; Update(); }
        void DisableReading() { m_events &= ~Poller::POLLIN; Update(); }
        void DisableWriting() { m_events &= ~Poller::POLLOUT; Update(); } 
        void DisableAll() { m_events = Poller::POLLNONE; Update(); }

        bool Reading() { return m_events & Poller::POLLIN; }
        bool Writing() { return m_events & Poller::POLLOUT; }

        int Fd() const { return m_fd; }
        uint32_t Events() const { return m_events; }
        void SetRevents(uint32_t revents) { m_revents = revents; }

        void Remove();

    private:
        void Update();

    private:

        EventLoop *m_event_loop;

        const int m_fd;
        uint32_t m_events;
        uint32_t m_revents;

        EventsCallback m_events_callback;
};

}

}
#endif
