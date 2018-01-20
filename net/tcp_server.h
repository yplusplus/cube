#ifndef __CUBE_TCP_SERVER_H__
#define __CUBE_TCP_SERVER_H__

#include <memory>
#include <map>

#include "callbacks.h"
#include "inet_addr.h"
#include "event_loop.h"
#include "tcp_connection.h"

namespace cube {

namespace net {

class EventLoop;
class Acceptor;

// combine it into your class and provide a ConnectionCallback
class TcpServer {
    public:
        TcpServer(EventLoop *event_loop, const InetAddr &server_addr);
        ~TcpServer();

        // for tcp connection
        void SetConnectionCallback(const ConnectionCallback &cb) 
        { m_connection_callback = cb; }

        bool Start();
        void Stop();

        const InetAddr &ServerAddr() const { return m_server_addr; }

        const std::string &ErrMsg() const { return m_err_msg; }
    private:
        void OnAccept(int sockfd);

        void RemoveConnection(TcpConnectionPtr conn);

    private:
        EventLoop *m_event_loop;

        const InetAddr &m_server_addr;

        std::unique_ptr<Acceptor> m_acceptor;

        ConnectionCallback m_connection_callback;
        std::string m_err_msg;

        std::map<uint64_t, TcpConnectionPtr> m_conns_map;
};

}

}
#endif
