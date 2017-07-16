#ifndef __CUBE_HTTP_CLI_CONNECTION_H__
#define __CUBE_HTTP_CLI_CONNECTION_H__

#include <memory>

#include "net/tcp_connection.h"
#include "http_response.h"

namespace cube {

namespace http {

class HTTPRequest;

class HTTPClientConnection : public std::enable_shared_from_this<HTTPClientConnection> {
    public:
        typedef std::function<void(std::shared_ptr<HTTPClientConnection>, const HTTPResponse *)> ResponseCallback;
        typedef std::function<void(std::shared_ptr<HTTPClientConnection>)> DisconnectionCallback;

        HTTPClientConnection(::cube::net::EventLoop *event_loop, ::cube::net::TcpConnectionPtr conn);
        ~HTTPClientConnection();

        void SetDisconnectCallback(const DisconnectionCallback &cb) { m_disconnect_callback = cb; }
        bool SendRequest(const HTTPRequest &request, const ResponseCallback &response_callback);

        uint64_t Id() const { return m_conn->Id(); }
        bool Requesting() const { return m_requesting; }
        void Close() { m_conn->Close(); }
        bool Closed() const { return m_conn->Closed(); }

        const ::cube::net::InetAddr LocalAddr() const { return m_conn->LocalAddr(); }
        const ::cube::net::InetAddr PeerAddr() const { return m_conn->PeerAddr(); }

    private:
        void OnConnect(::cube::net::TcpConnectionPtr conn, int status);
        void OnDisconnect(::cube::net::TcpConnectionPtr conn);
        void OnHeaders(::cube::net::TcpConnectionPtr conn, Buffer *buffer);
        bool ParseHeaders(Buffer *buffer);
        void OnBody(::cube::net::TcpConnectionPtr conn, Buffer *buffer);
        void HandleResponse();

    private:
        ::cube::net::EventLoop *m_event_loop;

        ::cube::net::TcpConnectionPtr m_conn;

        bool m_requesting;

        HTTPResponse m_response;
        ResponseCallback m_response_callback;

        DisconnectionCallback m_disconnect_callback;
};

typedef std::shared_ptr<HTTPClientConnection> HTTPClientConnectionPtr;

}

}

#endif
