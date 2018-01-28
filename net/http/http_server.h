#ifndef __CUBE_HTTP_SERVER_H__
#define __CUBE_HTTP_SERVER_H__

#include "net/tcp_server.h"
#include "http_connection.h"

namespace cube {

namespace http {

class HTTPServer {
    public:
        HTTPServer(::cube::net::EventLoop *event_loop,
                const ::cube::net::InetAddr &server_addr);
        ~HTTPServer();

        void SetRequestCallback(const RequestCallback &cb) { m_request_callback = cb; }
        bool Start();
        void Stop();

        const ::cube::net::InetAddr &ServerAddr() const { return m_server.ServerAddr(); }

        bool KeepAlive() const { return m_enable_keepalive; }
        void SetKeepAlive(bool on) { m_enable_keepalive = on; }

    private:
        void OnConnection(::cube::net::TcpConnectionPtr conn);
        void OnRequest(HTTPConnectionPtr conn, const HTTPRequest &request);

    private:
        ::cube::net::EventLoop *m_event_loop;

        ::cube::net::TcpServer m_server;

        RequestCallback m_request_callback;

        bool m_enable_keepalive;
};

}

}

#endif
