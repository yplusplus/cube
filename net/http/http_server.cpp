#include "net/event_loop.h"
#include "net/inet_addr.h"
#include "net/buffer.h"
#include "http_connection.h"
#include "http_request.h"
#include "http_response.h"
#include "http_server.h"

using namespace std::placeholders;

namespace cube {

namespace http {

HTTPServer::HTTPServer(EventLoop *event_loop,
        const InetAddr &server_addr) 
    : m_event_loop(event_loop),
    m_server(m_event_loop, server_addr),
    m_enable_keepalive(true) {

    m_server.SetNewConnectionCallback(std::bind(&HTTPServer::OnNewConnection, this, _1));
}

HTTPServer::~HTTPServer() {
}

bool HTTPServer::Start() {
    return m_server.Start();
}

void HTTPServer::Stop() {
    m_server.Stop();
}

void HTTPServer::OnNewConnection(TcpConnectionPtr conn) {
    // a new connection coming
    HTTPConnectionPtr http_conn(new HTTPConnection(
                m_event_loop, this, conn,
                std::bind(&HTTPServer::OnRequest, this, _1, _2)));
    m_conns[conn->Id()] = std::move(http_conn);
}

void HTTPServer::RemoveConnection(HTTPConnectionPtr conn) {
    m_conns.erase(conn->Id());
}

void HTTPServer::OnRequest(HTTPConnectionPtr conn, const HTTPRequest &request) {
    assert(m_request_callback);
    m_request_callback(conn, request);
}

}

}
