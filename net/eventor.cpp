#include "base/logging.h"

#include "eventor.h"
#include "event_loop.h"

namespace cube {

Eventor::Eventor(EventLoop *event_loop, int fd)
    : m_event_loop(event_loop),
    m_fd(fd),
    m_events(0),
    m_revents(0) {
}

Eventor::~Eventor() {
}

void Eventor::HandleEvents() {
    uint32_t revents = m_revents;
    m_revents = 0;
    m_events_callback(revents);
}

void Eventor::Update() {
    m_event_loop->UpdateEvents(this);
}

void Eventor::Remove() {
    m_events = 0;
    m_event_loop->RemoveEvents(this);
}

}
