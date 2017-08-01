#include "base/logging.h"

#include "net/event_loop.h"
#include "net/socket.h"
#include "net/connector.h"

#include "http_client.h"
#include "http_cli_connection.h"

using namespace std::placeholders;

namespace cube {

namespace http {

HTTPClient::HTTPClient(::cube::net::EventLoop *event_loop)
    : m_event_loop(event_loop) {
}

HTTPClient::~HTTPClient() {
    // close all connections
    auto idle_conns = std::move(m_idle_conns);
    for (auto it = idle_conns.begin(); it != idle_conns.end(); it++) {
        for (auto conn_it = it->second.begin(); conn_it != it->second.end(); conn_it++) {
            conn_it->second->Close();
        }
    }
    m_idle_conns.clear();

    auto requesting_conns = std::move(m_requesting_conns);
    for (auto it = requesting_conns.begin(); it != requesting_conns.end(); it++) {
        it->second->Close();
    }
}

HTTPClientConnectionPtr HTTPClient::GetConn(const ::cube::net::InetAddr &addr) {
    HTTPClientConnectionPtr conn;

    auto &idle_list = m_idle_conns[addr.IpPort()];
    if (!idle_list.empty()) {
        conn.swap(idle_list.begin()->second);
        idle_list.erase(idle_list.begin());
    }
    if (idle_list.empty()) m_idle_conns.erase(addr.IpPort());

    // fast path: get a idle connection
    if (conn) {
        M_LOG_INFO("get a idle connection[%lu] LocalAddr[%s] PeerAddr[%s]",
                conn->Id(),
                conn->LocalAddr().IpPort().c_str(),
                conn->PeerAddr().IpPort().c_str());
        return conn;
    }

    // slow path: create a new connection
    if (!conn) {
        auto tcp_conn = ::cube::net::Connector::Connect(m_event_loop, addr);
        if (tcp_conn) {
            conn = std::make_shared<HTTPClientConnection>(m_event_loop, tcp_conn);
        } else {
            // TODO retry ??
        }
    }
    return conn;
}

void HTTPClient::PutConn(HTTPClientConnectionPtr conn) {
    M_LOG_INFO("put a idle connection PeerAddr[%s], Id[%lu]",
            conn->PeerAddr().IpPort().c_str(), conn->Id());

    auto &idle_list = m_idle_conns[conn->PeerAddr().IpPort()];
    // at most 16 idle conns per [Ip:Port]
    if (idle_list.size() < 16) {
        idle_list[conn->Id()] = conn;
    } else {
        conn->Close();
    }
}

int HTTPClient::Send(const ::cube::net::InetAddr &addr, const HTTPRequest &request, const ResponseCallback &response_callback, int64_t timeout_ms /* = 2000 */) {
    HTTPClientConnectionPtr conn = GetConn(addr);
    if (!conn) {
        // no idle conns and connect failed??
        // run callback in next loop
        M_LOG_WARN("get conn failed");
        return CUBE_ERR;
    }

    if (!conn->SendRequest(request,
                std::bind(&HTTPClient::OnResponse, this, response_callback, _1, _2))) {
        // send failed
        // run callback in next loop
        M_LOG_WARN("send request failed");
        return CUBE_ERR;
    }
    m_requesting_conns[conn->Id()] = conn;

    // add timeout timer
    uint64_t timer_id = m_event_loop->RunAfter(
            std::bind(&HTTPClient::OnResponseTimeout,
                this, conn), timeout_ms);
    m_timeouts[conn->Id()] = timer_id;

    return CUBE_OK;
}

void HTTPClient::OnDisconnect(HTTPClientConnectionPtr conn) {
    auto it = m_idle_conns.find(conn->PeerAddr().IpPort());
    if (it == m_idle_conns.end())
        return;

    M_LOG_TRACE("remove conn[%lu] from idle list", conn->Id());

    it->second.erase(conn->Id());

    // remove empty map
    if (it->second.empty())
        m_idle_conns.erase(it);
}

void HTTPClient::OnResponse(const ResponseCallback &response_callback, HTTPClientConnectionPtr conn, const HTTPResponse *response) {
    // remove from requesting connections
    m_requesting_conns.erase(conn->Id());

    // remove timer if not time out
    auto it = m_timeouts.find(conn->Id());
    if (it != m_timeouts.end()) {
        m_event_loop->CancelTimer(it->second);
        m_timeouts.erase(it);
    }

    // put back to idle list
    if (!conn->Closed())
        PutConn(conn);

    response_callback(response);
}

void HTTPClient::OnResponseTimeout(HTTPClientConnectionPtr conn) {
    M_LOG_WARN("OnResponseTimeout conn[%lu] local_addr[%s], peer_addr[%s]",
            conn->Id(),
            conn->LocalAddr().IpPort().c_str(),
            conn->PeerAddr().IpPort().c_str());

    // we know timeout-timer has been activated
    // so remove from timeouts
    m_timeouts.erase(conn->Id());

    // call back by calling Close()
    conn->Close();
}

}

}
