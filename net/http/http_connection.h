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
        HTTPConnection(HTTPServer *server,
                const ::cube::net::TcpConnectionPtr &conn,
                const RequestCallback &request_callback);
        ~HTTPConnection();

        // send response
        // continue to handle request when keep alive 
        // otherwise, close the connection 
        bool SendResponse(HTTPResponse &response);
        uint64_t Id() const { return m_conn->Id(); }
        void Close();
        bool Closed() const { return m_conn->Closed(); }

        void OnConnection(::cube::net::TcpConnectionPtr conn);

    private:
        void OnHeaders(::cube::net::TcpConnectionPtr conn, Buffer *buffer);
        bool ParseHeaders(Buffer *buffer);
        void OnBody(::cube::net::TcpConnectionPtr conn, Buffer *buffer);

        void HandleRequest();
        void OnWriteComplete(::cube::net::TcpConnectionPtr conn, bool keep_alive);


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
