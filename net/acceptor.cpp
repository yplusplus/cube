#include "acceptor.h"

#include <assert.h>

#include "base/log.h"
#include "event_loop.h"
#include "eventor.h"
#include "socket.h"

using namespace std::placeholders;

namespace cube {

Acceptor::Acceptor(EventLoop *event_loop,
	const InetAddr &listen_addr,
    const AcceptCallback &accept_callback)
    : m_event_loop(event_loop),
    m_sock(), m_eventor(),
    m_listen_addr(listen_addr),
    m_accept_callback(accept_callback) {
}

Acceptor::~Acceptor() {
}

bool Acceptor::Listen() {
    int sockfd = sockets::CreateNonBlockStreamSocket();
    if (sockfd < 0) {
        return false;
    }
    m_sock.reset(new Socket(sockfd));
    m_sock->SetReuseAddr(true);
    if (!m_sock->BindAndListen(m_listen_addr, 1024)) {
        // error
        m_sock.reset();
        return false;
    }
    m_eventor.reset(new Eventor(m_event_loop, sockfd));
    m_eventor->SetEventsCallback(std::bind(&Acceptor::HandleEvents, this, _1));
    m_eventor->EnableReading();
    return true;
}

void Acceptor::Stop() {
    if (m_sock) {
        assert(m_eventor);
        m_eventor->Remove();
        m_eventor.reset();
        m_sock.reset();
    }
}

void Acceptor::HandleEvents(int revents) {
    if (revents & Poller::POLLIN) {
        HandleRead();
    }
}

void Acceptor::HandleRead() {
    int fd = m_sock->Accept();
    if (fd >= 0) {
        assert(m_accept_callback);
        m_accept_callback(fd);
    } else {
        // error
        LOG_ERROR("accept failed");
    }
}

}
