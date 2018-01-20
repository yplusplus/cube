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
    //auto idle_conns = std::move(m_idle_conns);
    for (auto it = m_idle_conns.begin(); it != m_idle_conns.end(); it++) {
        while (!it->second.empty()) {
            it->second.begin()->second->Close();
        }
    }
    m_idle_conns.clear();

    assert(m_requesting_conns.empty());
}

HTTPClientConnectionPtr HTTPClient::GetConn(const ::cube::net::InetAddr &addr) {
    HTTPClientConnectionPtr http_conn;

    auto &idle_list = m_idle_conns[addr.IpPort()];
    if (!idle_list.empty()) {
        http_conn.swap(idle_list.begin()->second);
        idle_list.erase(idle_list.begin());
    }
    if (idle_list.empty()) m_idle_conns.erase(addr.IpPort());

    // fast path: get a idle connection
    if (http_conn) {
        M_LOG_INFO("get a idle connection[%lu] LocalAddr[%s] PeerAddr[%s]",
                http_conn->Id(),
                http_conn->LocalAddr().IpPort().c_str(),
                http_conn->PeerAddr().IpPort().c_str());
        return http_conn;
    }

    // slow path: create a new connection
    if (!http_conn) {
        auto conn = ::cube::net::Connector::Connect(m_event_loop, addr);
        if (conn) {
            conn->EnableWriting();
            http_conn = std::make_shared<HTTPClientConnection>(conn);

            conn->SetCloseCallback(
                    std::bind(&HTTPClient::OnClose,
                        this, std::placeholders::_1));
        }
    }
    return http_conn;
}

void HTTPClient::PutConn(HTTPClientConnectionPtr http_conn) {
    M_LOG_INFO("put a idle connection PeerAddr[%s], Id[%lu]",
            http_conn->PeerAddr().IpPort().c_str(), http_conn->Id());

    auto &idle_list = m_idle_conns[http_conn->PeerAddr().IpPort()];
    // at most 16 idle conns per [Ip:Port]
    if (idle_list.size() < DEFAULT_MAX_IDLE_CONNS_NUMBER) {
        idle_list[http_conn->Id()] = http_conn;
    } else {
        http_conn->Close();
    }
}

int HTTPClient::Send(const ::cube::net::InetAddr &addr,
        const HTTPRequest &request,
        const ResponseCallback &callback,
        int64_t timeout_ms /* = 2000 */) {
    HTTPClientConnectionPtr conn = GetConn(addr);
    if (!conn) {
        // no idle conns and connect failed??
        M_LOG_WARN("get conn failed");
        return CUBE_ERR;
    }

    if (!conn->SendRequest(request,
                std::bind(&HTTPClient::OnResponse, this, callback, _1, _2))) {
        // send failed
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

int HTTPClient::Get(const ::cube::net::InetAddr &addr,
        const std::string &url,
        const ResponseCallback &callback,
        int64_t timeout_ms /* = 2000 */) {
    // TODO
    HTTPRequest request;
    request.SetURL(url);
    
    return Send(addr, request, callback, timeout_ms);
}

int HTTPClient::Post(const ::cube::net::InetAddr &addr,
        const std::string &url,
        const std::string &body,
        const ResponseCallback &callback,
        int64_t timeout_ms /* = 2000 */) {
    // TODO
    HTTPRequest request;
    request.SetURL(url);
    request.SetMethod("POST");
    if (body.length() > 0)
        request.Write(body);

    return Send(addr, request, callback, timeout_ms);
}

void HTTPClient::OnResponse(const ResponseCallback &callback, HTTPClientConnectionPtr conn, const HTTPResponse *response) {
    // remove from requesting connections
    m_requesting_conns.erase(conn->Id());

    // remove timer if has
    auto it = m_timeouts.find(conn->Id());
    if (it != m_timeouts.end()) {
        m_event_loop->CancelTimer(it->second);
        m_timeouts.erase(it);
    }

    // put back to idle list if not closed
    if (!conn->Closed())
        PutConn(conn);

    M_LOG_DEBUG("OnResponse conn[%lu] response %s NULL", conn->Id(), response == NULL ? "is" : "is not");
    callback(response);
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

void HTTPClient::OnClose(::cube::net::TcpConnectionPtr conn) {
    assert(m_requesting_conns.count(conn->Id()) == 0);

    const std::string peer_addr = conn->PeerAddr().IpPort().c_str();
    M_LOG_DEBUG("OnClose conn[%lu], in idle conns[%d]",
            conn->Id(), m_idle_conns[peer_addr].count(conn->Id()) == 0 ? 0 : 1);

    m_idle_conns[peer_addr].erase(conn->Id());
}

}

}
