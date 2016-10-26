#ifndef __CUBE_HTTP_CONNECTION_H__
#define __CUBE_HTTP_CONNECTION_H__

#include "net/tcp_connection.h"

#include "http_request.h"
#include "http_response.h"

namespace cube {

class EventLoop;

namespace http {

class HTTPServer;
class HTTPConnection;
typedef std::shared_ptr<HTTPConnection> HTTPConnectionPtr;
typedef std::function<void(HTTPConnectionPtr, const HTTPRequest &)> RequestCallback;

class HTTPConnection : public std::enable_shared_from_this<HTTPConnection> {
    public:
        HTTPConnection(EventLoop *event_loop, HTTPServer *server, const TcpConnectionPtr &conn, const RequestCallback &request_callback);
        ~HTTPConnection();

        // send response and set write-complete callback
        // continue to handle request when keep alive 
        // otherwise, close the http connection 
        bool SendResponse(const HTTPResponse &response);
        uint64_t Id() const { return m_conn->Id(); }

    private:
        void OnConnect(TcpConnectionPtr conn, int status);
        void OnDisconnect(TcpConnectionPtr conn);
        
        void OnHeaders(TcpConnectionPtr conn, Buffer *buffer);
        bool ParseHeaders(Buffer *buffer);
        void OnBody(TcpConnectionPtr conn, Buffer *buffer);

        void HandleRequest();
        void OnWriteComplete(TcpConnectionPtr conn, bool keep_alive);

        void Close();
        bool Closed() const;

    private:
        EventLoop *m_event_loop;

        HTTPServer *m_server;

        // associated tcp-connection
        TcpConnectionPtr m_conn;

        RequestCallback m_request_callback;

        HTTPRequest m_request;
};

}

}

#endif
