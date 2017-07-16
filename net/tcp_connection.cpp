#include <unistd.h>

#include "tcp_connection.h"

#include "base/time_util.h"
#include "base/logging.h"
#include "base/strings.h"
#include "event_loop.h"
#include "eventor.h"
#include "socket.h"

using namespace std::placeholders;

namespace cube {

namespace net {

uint64_t TcpConnection::m_next_conn_id(1);

TcpConnection::TcpConnection(EventLoop *event_loop,
        int sockfd,
        const InetAddr &local_addr,
        const InetAddr &peer_addr)
    : m_event_loop(event_loop),
    m_conn_id(__sync_fetch_and_add(&m_next_conn_id, 1)),
    m_sock(new Socket(sockfd)),
    m_eventor(new Eventor(m_event_loop, sockfd)),
    m_local_addr(local_addr),
    m_peer_addr(peer_addr),
    m_state(ConnState_Connected),
    m_read_bytes(-1),
    m_last_active_time(TimeUtil::CurrentTime()) {

    m_eventor->SetEventsCallback(std::bind(&TcpConnection::HandleEvents, this, _1));

    m_sock->SetKeepAlive(true);
    m_sock->SetNonBlocking(true);
}

TcpConnection::~TcpConnection() {
    M_LOG_DEBUG("~TcpConnection conn[%lu]", m_conn_id);
    // ensure to release all resources.
    assert(Closed());
    if (!Closed())
        Close();
}

void TcpConnection::ReadAny(const ReadCallback &cb) {
    return ReadBytes(1, cb);
}

void TcpConnection::ReadBytes(size_t read_bytes, const ReadCallback &cb) {
    assert(cb);
    if (Closed()) return;
    if (m_input_buffer.ReadableBytes() >= read_bytes) {
        m_event_loop->Post(std::bind(cb, shared_from_this(), &m_input_buffer));
        return;
    }
    m_read_bytes = read_bytes;
    m_read_callback = cb;
    if (!m_eventor->Reading()) {
        EnableReading();
    }
    return;
}

void TcpConnection::ReadUntil(const std::string &delimiter, const ReadCallback &cb) {
    assert(cb);
    assert(!delimiter.empty());
    if (Closed()) return;
    if (m_input_buffer.Find(delimiter) != NULL) {
        m_event_loop->Post(std::bind(cb, shared_from_this(), &m_input_buffer));
        return;
    }
    m_read_delimiter = delimiter;
    m_read_callback = cb;
    if (!m_eventor->Reading()) {
        EnableReading();
    }
    return;
}

bool TcpConnection::Write(const std::string &str) {
    return Write(str.data(), str.length(), NULL);
}

bool TcpConnection::Write(const char *data, size_t len) {
    return Write(data, len, NULL);
}

bool TcpConnection::Write(const std::string &str, const WriteCompleteCallback &cb) {
    return Write(str.data(), str.length(), cb);
}

bool TcpConnection::Write(const char *data, size_t len, const WriteCompleteCallback &cb) {
    // Not allow to send data when closed
    if (Closed()) {
        strings::FormatString(m_err_msg, "tcp connection[%lu] is closed", Id());
        return false;
    }

    if (len == 0) {
        // It is always ok to send 0-length data
        if (cb) {
            m_event_loop->Post(std::bind(cb, shared_from_this()));
        }
        return true;
    }

    // send data directly when no pending data to send
    if (m_state == ConnState_Connected && m_output_buffer.ReadableBytes() == 0) {
        int nwrote = ::write(m_eventor->Fd(), data, len);
        if (nwrote < 0) {
            // send data directly failed
            // write data to output buffer and wait for POLLIN event
        } else if (nwrote >= static_cast<int>(len)) {
            // send all data directly
            if (cb)
                m_event_loop->Post(std::bind(cb, shared_from_this()));
            return true;
        } else {
            // send some data directly but not all,
            // write the rest to output buffer
            data += nwrote;
            len -= nwrote;
        }
    }

    m_write_complete_callback = cb;
    //SetWriteCompleteCallback(cb);
    m_output_buffer.Append(data, len);
    if (!m_eventor->Writing())
        m_eventor->EnableWriting();
    return true;
}

void TcpConnection::Close() {
    HandleClose();
}

void TcpConnection::CloseAfter(int64_t delay_ms) {
    m_event_loop->AssertInLoopThread();
    m_event_loop->RunAfter(std::bind(&TcpConnection::Close, shared_from_this()), delay_ms);
}

void TcpConnection::EnableReading() {
    if (Closed()) return;
    m_eventor->EnableReading();
}

void TcpConnection::DisableReading() {
    if (Closed()) return;
    m_eventor->DisableReading();
}

void TcpConnection::Initialize() {
    TcpConnectionPtr conn(shared_from_this());
    m_eventor->EnableReading();

    // For server
    if (m_state == ConnState_Connected) {
        if (m_connect_callback) {
            m_connect_callback(conn, CUBE_OK);
        }
    }
}

void TcpConnection::OnRead() {
    ReadCallback read_callback;
    if (m_read_bytes > 0 && static_cast<int>(m_input_buffer.ReadableBytes()) >= m_read_bytes) {
        // ReadAny or ReadBytes
        read_callback = std::move(m_read_callback);
        m_read_bytes = -1;
    } else if (!m_read_delimiter.empty() && m_input_buffer.Find(m_read_delimiter) != NULL) {
        // ReadUntil
        read_callback = std::move(m_read_callback);
        m_read_delimiter.clear();
    }
    if (read_callback)
        read_callback(shared_from_this(), &m_input_buffer);
    return;
}

void TcpConnection::HandleRead() {
    m_event_loop->AssertInLoopThread();

    if (Closed()) return;

    bool has_error = false;
    if (m_eventor->Reading()) {
        if (m_state == ConnState_Connecting) {
            if (HandleConnect() != CUBE_OK) {
                return;
            }
        } else {
            ssize_t nread = m_input_buffer.ReadFromFd(m_sock->Fd());
            if (nread < 0) {
                // error
                has_error = true;
            } else if (nread == 0) {
                // close by peer
                M_LOG_DEBUG("conn[%lu] closed by peer", Id());
                HandleClose();
            } else {
                OnRead();
            }
        }
    } else {
        // error
        has_error = true;
    }

    if (has_error) 
        HandleError();
}

void TcpConnection::HandleEvents(int revents) {
    // prevent connection being destroyed in HandleXXXX()
    TcpConnectionPtr guard(shared_from_this());

    m_last_active_time = TimeUtil::CurrentTime();

    if (revents & Poller::POLLERR) {
        HandleError();
    }

    if ((revents & Poller::POLLHUB) && (revents & ~Poller::POLLIN)) {
        HandleClose();
    }

    if (revents & Poller::POLLIN) {
        HandleRead();
    }

    if (revents & Poller::POLLOUT) {
        HandleWrite();
    }
}

int TcpConnection::HandleConnect() {
    m_event_loop->AssertInLoopThread();

    if (Closed()) return CUBE_ERR;

    M_LOG_DEBUG("conn[%lu] connecting!", Id());

    int saved_error = 0;
    int ret = sockets::GetSocketError(m_sock->Fd(), saved_error);
    if (ret || saved_error) {
        M_LOG_ERROR("conn[%lu] error on connecting, error=%d", m_conn_id, saved_error);
        if (m_connect_callback)
            m_connect_callback(shared_from_this(), CUBE_ERR);
        HandleError();
        return CUBE_ERR;
    }

    m_state = ConnState_Connected;
    if (m_connect_callback)
        m_connect_callback(shared_from_this(), CUBE_OK);
    return CUBE_OK;
}

void TcpConnection::HandleWrite() {
    m_event_loop->AssertInLoopThread();

    if (Closed()) return;

    bool has_error = false;
    if (m_eventor->Writing()) {
        //LOG_DEBUG("conn[%lu] writing!", Id());
        if (m_state == ConnState_Connecting) {
            if (HandleConnect() != CUBE_OK) {
                return;
            }
        } else {
            int nwrote = ::write(m_eventor->Fd(),
                    m_output_buffer.Peek(),
                    m_output_buffer.ReadableBytes());
            if (nwrote < 0) {
                if (errno == EINTR) {
                    // it is ok.
                } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    // it is ok.
                } else {
                    // error
                    has_error = true;
                }
            } else {
                m_output_buffer.Retrieve(nwrote);

                // write completely
                if (m_output_buffer.ReadableBytes() == 0) {
                    m_eventor->DisableWriting();
                    if (m_write_complete_callback)
                        m_write_complete_callback(shared_from_this());
                }
            }
        }
    } else {
        // error
        has_error = true;
    }

    if (has_error) 
        HandleError();
}

void TcpConnection::HandleClose() {
    m_event_loop->AssertInLoopThread();

    // has closed??
    if (Closed()) return;
    m_state = ConnState_Disconnected;
    m_write_complete_callback = NULL;
    m_read_callback = NULL;

    // remove events from poller
    m_eventor->Remove();

    if (m_disconnect_callback)
        m_disconnect_callback(shared_from_this());
}

void TcpConnection::HandleError() {
    m_event_loop->AssertInLoopThread();
    HandleClose();
}

}

}
