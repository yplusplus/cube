#include "base/logging.h"

#include "net/event_loop.h"
#include "net/socket.h"
#include "net/connector.h"

#include "http_client.h"
#include "http_cli_connection.h"

using namespace std::placeholders;

namespace cube {

namespace http {

HTTPClient::HTTPClient(EventLoop *event_loop)
    : m_event_loop(event_loop) {
}

HTTPClient::~HTTPClient() {
    // close all connections
    for (auto it = m_idle_conns.begin(); it != m_idle_conns.end(); it++) {
        for (auto conn_it = it->second.begin(); conn_it != it->second.end(); conn_it++) {
            (*conn_it)->Close();
        }
    }
    m_idle_conns.clear();

    auto requesting_conns = std::move(m_requesting_conns);
    for (auto it = requesting_conns.begin(); it != requesting_conns.end(); it++) {
        it->second->Close();
    }
}

HTTPClientConnectionPtr HTTPClient::GetConn(const InetAddr &addr) {
    HTTPClientConnectionPtr conn;

    auto &idle_list = m_idle_conns[addr.IpPort()];
    while (!idle_list.empty()) {
        conn.swap(idle_list.back());
        idle_list.pop_back();
        if (conn->Closed()) {
            // connection has been closed
            conn.reset();
        } else {
            // find a idle connection
            M_LOG_INFO("found a idle connection PeerAddr[%s], Id[%lu]",
                    conn->PeerAddr().IpPort().c_str(), conn->Id());
            break;
        }
    }
    if (idle_list.empty()) m_idle_conns.erase(addr.IpPort());

    // create a new one when no idle connections
    if (!conn) {
        int sockfd = -1;
        int ret = Connector::Connect(addr, sockfd);
        // retry ??
        if (ret != CUBE_OK) {
            // connect failed
            return conn;
        }
        TcpConnectionPtr tcp_conn(new TcpConnection(
                    m_event_loop,
                    sockfd,
                    sockets::GetLocalAddr(sockfd),
                    sockets::GetPeerAddr(sockfd)));

        conn = std::make_shared<HTTPClientConnection>(m_event_loop, tcp_conn);
    }
    return conn;
}

void HTTPClient::PutConn(HTTPClientConnectionPtr conn) {
    M_LOG_INFO("put a idle connection PeerAddr[%s], Id[%lu]",
            conn->PeerAddr().IpPort().c_str(), conn->Id());

    auto &idle_list = m_idle_conns[conn->PeerAddr().IpPort()];
    // at most 16 idle conns per [Ip:Port]
    if (idle_list.size() < 16) {
        idle_list.push_back(std::move(conn));
    } else {
        conn->Close();
    }
}

void HTTPClient::Send(const InetAddr &addr, const HTTPRequest &request, const ResponseCallback &response_callback) {
    HTTPClientConnectionPtr conn;
    conn = GetConn(addr);
    if (!conn) {
        // no idle conns and connect failed??
        // run callback in next loop
        m_event_loop->Post(std::bind(response_callback, (const HTTPResponse *)NULL));
        return;
    }

    if (!conn->SendRequest(request,
                std::bind(&HTTPClient::OnResponse, this, response_callback, _1, _2))) {
        // send failed
        // run callback in next loop
        m_event_loop->Post(std::bind(response_callback, (const HTTPResponse *)NULL));
        return;
    }
    m_requesting_conns[conn->Id()] = conn;
}

void HTTPClient::OnDisconnect(HTTPClientConnectionPtr conn) {
    // connection has been closed, lazy-release in GetConn()
}

void HTTPClient::OnResponse(const ResponseCallback &response_callback, HTTPClientConnectionPtr conn, const HTTPResponse *response) {
    // remove from requesting connections
    m_requesting_conns.erase(conn->Id());

    // put back to idle list
    if (!conn->Closed())
        PutConn(conn);

    response_callback(response);
}

}

}
