#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "udp_connection.h"

#include "event_loop.h"
#include "eventor.h"
#include "socket.h"
#include "base/strings.h"

namespace cube {

namespace net {

UdpConnection::UdpConnection(EventLoop *event_loop,
        int sockfd,
        size_t buffer_max_length)
    : m_event_loop(event_loop),
    m_sock(new Socket(sockfd)),
    m_eventor(new Eventor(m_event_loop, sockfd)),
    m_input_buffer_max_length(buffer_max_length),
    m_input_buffer(new char[m_input_buffer_max_length]),
    m_input_buffer_length(0),
    m_closed(false) {

    m_eventor->SetEventsCallback(std::bind(&UdpConnection::HandleEvents, this, std::placeholders::_1));
}

UdpConnection::~UdpConnection() {
    if (m_input_buffer) {
        delete[] m_input_buffer;
        m_input_buffer = NULL;
    }
}

void UdpConnection::EnableReading() {
    m_eventor->EnableReading();
}

void UdpConnection::DisableReading() {
    m_eventor->DisableReading();
}

bool UdpConnection::SendTo(const InetAddr &dest_addr, const char *data, size_t len) {
    ssize_t ret = ::sendto(m_sock->Fd(), data, len, 0,
            dest_addr.SockAddr(), sizeof(dest_addr.SockAddrIn()));

    if (ret < (ssize_t)len) {
        if (ret == -1) {
            strings::FormatString(m_err_msg, "udp connection sendto dest[%s] failed, msg=%s",
                    dest_addr.IpPort().c_str(), strerror(errno));
        } else {
            strings::FormatString(m_err_msg, "udp connection expects to sendto dest[%s] %d data, but actually sends %d", dest_addr.IpPort().c_str(), (int)len, (int)ret);
        }
        return false;
    }
    return true;
}

void UdpConnection::Close() {
    HandleClose();
}

void UdpConnection::HandleEvents(int revents) {
    if (revents & Poller::POLLIN) {
        HandleRead();
    }
}

void UdpConnection::HandleRead() {
    m_event_loop->AssertInLoopThread();

    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);
    while (true) {
        ssize_t ret = ::recvfrom(
                m_sock->Fd(),
                m_input_buffer,
                m_input_buffer_max_length,
                0,
                (struct sockaddr *)&src_addr,
                &addrlen);

        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            } else {
                strings::FormatString(m_err_msg, "udp connection[%d] recvfrom failed, msg=%s",
                        m_sock->Fd(), strerror(errno));
                break;
            }
        }
        assert(m_read_callback);
        InetAddr peer_addr(src_addr);
        m_input_buffer_length = (size_t)ret;
        m_read_callback(shared_from_this(), m_input_buffer, m_input_buffer_length, peer_addr);
    }
}

void UdpConnection::HandleClose() {
    m_event_loop->AssertInLoopThread();

    // has closed??
    if (Closed()) return;
    m_closed = true;
    m_read_callback = NULL;
    
    // remove events from poller
    m_eventor->Remove();
}

}

}
