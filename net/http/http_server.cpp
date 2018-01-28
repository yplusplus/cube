#include "base/buffer.h"
#include "net/event_loop.h"
#include "net/inet_addr.h"
#include "http_connection.h"
#include "http_request.h"
#include "http_response.h"
#include "http_server.h"

using namespace std::placeholders;

namespace cube {

namespace http {

HTTPServer::HTTPServer(::cube::net::EventLoop *event_loop,
        const ::cube::net::InetAddr &server_addr) 
    : m_event_loop(event_loop),
    m_server(m_event_loop, server_addr),
    m_enable_keepalive(true) {

    m_server.SetConnectionCallback(std::bind(&HTTPServer::OnConnection, this, _1));
}

HTTPServer::~HTTPServer() {
}

bool HTTPServer::Start() {
    return m_server.Start();
}

void HTTPServer::Stop() {
    // stop tcp server
    m_server.Stop();
}

void HTTPServer::OnConnection(::cube::net::TcpConnectionPtr conn) {
    assert(conn->GetState() == ::cube::net::TcpConnection::ConnState_Connected);

    switch (conn->GetState()) {
        case ::cube::net::TcpConnection::ConnState_Connected:
        {
            // a new connection coming
            HTTPConnectionPtr http_conn(new HTTPConnection(this, conn,
                        std::bind(&HTTPServer::OnRequest, this, _1, _2)));

            // reset connection callback to refer http conn
            conn->SetConnectionCallback(std::bind(&HTTPConnection::OnConnection, http_conn, std::placeholders::_1));
        }
        break;
        default: // do nothing
            break;
    }
}

void HTTPServer::OnRequest(HTTPConnectionPtr conn, const HTTPRequest &request) {
    assert(m_request_callback);
    m_request_callback(conn, request);
}

}

}
