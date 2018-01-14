#ifndef __CUBE_UDP_CONNECTION_H__
#define __CUBE_UDP_CONNECTION_H__

#include <memory>
#include <string>

#include "inet_addr.h"

#include "callbacks.h"

namespace cube {

namespace net {

class EventLoop;
class Eventor;
class Socket;

class UdpConnection : public std::enable_shared_from_this<UdpConnection> {

    public:
        UdpConnection(EventLoop *event_loop, int sockfd, size_t buffer_max_length);
        ~UdpConnection();

        // callback setter
        void SetReadCallback(const UdpReadCallback &cb) { m_read_callback = cb; }

        void EnableReading();
        void DisableReading();

        bool SendTo(const InetAddr &dest_addr, const char *data, size_t len);

        const std::string &ErrMsg() const { return m_err_msg; }

        void Close();
        bool Closed() const { return m_closed; }

    private:
        void HandleEvents(int revents);
        void HandleRead();
        void HandleClose();

    private:

        EventLoop *m_event_loop;

        std::unique_ptr<Socket> m_sock;
        std::unique_ptr<Eventor> m_eventor;

        size_t m_input_buffer_max_length;
        char *m_input_buffer;
        size_t m_input_buffer_length;

        UdpReadCallback m_read_callback;

        std::string m_err_msg;

        bool m_closed;
};

typedef std::shared_ptr<UdpConnection> UdpConnectionPtr;
typedef std::weak_ptr<UdpConnection> UdpConnectionWPtr;

}

}

#endif
