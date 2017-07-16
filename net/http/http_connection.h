#ifndef __CUBE_HTTP_CONNECTION_H__
#define __CUBE_HTTP_CONNECTION_H__

#include "net/event_loop.h"
#include "net/tcp_connection.h"

#include "http_request.h"
#include "http_response.h"

namespace cube {

namespace http {

class HTTPServer;
class HTTPConnection;
typedef std::shared_ptr<HTTPConnection> HTTPConnectionPtr;
typedef std::function<void(HTTPConnectionPtr, const HTTPRequest &)> RequestCallback;

class HTTPConnection : public std::enable_shared_from_this<HTTPConnection> {
    public:
        HTTPConnection(::cube::net::EventLoop *event_loop,
                HTTPServer *server,
                const ::cube::net::TcpConnectionPtr &conn,
                const RequestCallback &request_callback);
        ~HTTPConnection();

        // send response and set write-complete callback
        // continue to handle request when keep alive 
        // otherwise, close the http connection 
        bool SendResponse(const HTTPResponse &response);
        uint64_t Id() const { return m_conn->Id(); }

    private:
        void OnConnect(::cube::net::TcpConnectionPtr conn, int status);
        void OnDisconnect(::cube::net::TcpConnectionPtr conn);
        
        void OnHeaders(::cube::net::TcpConnectionPtr conn, Buffer *buffer);
        bool ParseHeaders(Buffer *buffer);
        void OnBody(::cube::net::TcpConnectionPtr conn, Buffer *buffer);

        void HandleRequest();
        void OnWriteComplete(::cube::net::TcpConnectionPtr conn, bool keep_alive);

        void Close();
        bool Closed() const;

    private:
        ::cube::net::EventLoop *m_event_loop;

        HTTPServer *m_server;

        // associated tcp-connection
        ::cube::net::TcpConnectionPtr m_conn;

        RequestCallback m_request_callback;

        HTTPRequest m_request;
};

}

}

#endif
