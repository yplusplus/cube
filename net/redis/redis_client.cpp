// hiredis headers
#include "async.h"
#include "hiredis.h"

#include "base/log.h"
#include "net/event_loop.h"
#include "net/inet_addr.h"
#include "net/socket.h"
#include "redis_connection.h"
#include "redis_client.h"

using namespace std::placeholders;

namespace cube {

namespace redis {

RedisClient::RedisClient(EventLoop *event_loop)
    : m_event_loop(event_loop) {
}

RedisClient::~RedisClient() {
    LOG_DEBUG("");
    HandleClose();
}

RedisConnectionPtr RedisClient::Connect(const InetAddr &server_addr) {
    RedisConnectionPtr conn;
    redisAsyncContext *redis_context = redisAsyncConnect(
            server_addr.Ip().c_str(), server_addr.Port());
    if (redis_context == NULL) {
        // error
        return conn;
    }

    if (redis_context->err) {
        // error
        redisAsyncFree(redis_context);
        return conn;
    }

    LOG_DEBUG("redis client connect successfully");

    int sockfd = redis_context->c.fd;
    conn = std::make_shared<RedisConnection>(
            m_event_loop, redis_context,
            sockets::GetLocalAddr(sockfd), sockets::GetPeerAddr(sockfd));

    conn->SetDisconnectCallback(std::bind(&RedisClient::OnDisconnect, this, _1));
    conn->Initialize();
    return conn;
}

RedisConnectionPtr RedisClient::GetConn(const InetAddr &addr) {
    RedisConnectionPtr conn;
    auto &idle_list = m_idle_conns[addr.IpPort()];
    while (!idle_list.empty()) {
        conn.swap(idle_list.back());
        idle_list.pop_back();
        if (conn->Closed()) {
            // connection has been closed
            conn.reset();
        } else {
            // found a idle connection
            LOG_INFO("found a idle connection PeerAddr[%s] conn[%lu]", conn->PeerAddr().IpPort().c_str(), conn->Id());
            break;
        }
    }
    if (idle_list.empty()) m_idle_conns.erase(addr.IpPort());

    // create a new one when no idle connections
    if (!conn) {
        conn = Connect(addr);
    }

    return conn;
}

void RedisClient::PutConn(RedisConnectionPtr conn) {
    auto &idle_list = m_idle_conns[conn->PeerAddr().IpPort()];
    // at most 16 idle conns per [Ip:Port]
    if (idle_list.size() < 16) {
        idle_list.push_back(std::move(conn));
    } else {
        conn->Close();
    }
}

void RedisClient::IssueCommand(const InetAddr &server_addr,
        const RedisReplyCallback &callback, const char *format, ...) {

    RedisConnectionPtr conn = GetConn(server_addr);
    if (!conn) {
        // no idle connections and connect failed??
        // run callback in next loop
        m_event_loop->Post(std::bind(callback, (redisReply *)NULL));
        return;
    }

    va_list ap;
    va_start(ap, format);
    int status = conn->IssueCommand(
            std::bind(&RedisClient::OnRedisReply, this, callback, conn, _1), format, ap);
    va_end(ap);
    if (status != CUBE_OK) {
        // issue command failed, run callback in next loop
        // and close the connection
        m_event_loop->Post(std::bind(callback, (redisReply *)NULL));
        conn->Close();
        return;
    }
    m_requesting_conns[conn->Id()] = conn;

    LOG_DEBUG("IssueCommand successfully");
}

void RedisClient::OnDisconnect(RedisConnectionPtr conn) {
    // connection has been closed, lazy-release in GetConn()
}

void RedisClient::OnRedisReply(RedisReplyCallback callback, RedisConnectionPtr conn, redisReply *reply) {
    // remove from requesting connections
    m_requesting_conns.erase(conn->Id());

    // put back to idle list
    if (!conn->Closed()) 
        PutConn(conn);

    callback(reply);
}

void RedisClient::HandleClose() {
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

}

}
