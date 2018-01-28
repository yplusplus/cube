#include <assert.h>
#include <string.h>

// hiredis headers
#include "async.h"
#include "hiredis.h"

#include "base/logging.h"
#include "net/event_loop.h"
#include "net/inet_addr.h"
#include "net/socket.h"
#include "redis_connection.h"
#include "redis_client.h"

using namespace std::placeholders;

namespace cube {

namespace redis {

RedisClient::RedisClient(::cube::net::EventLoop *event_loop,
        const ::cube::net::InetAddr &addr,
        const std::string &passwd)
    : m_event_loop(event_loop),
    m_redis_addrs(1, addr),
    m_redis_addr_idx(0),
    m_passwd(passwd) {

    assert(!m_redis_addrs.empty());
}

RedisClient::RedisClient(::cube::net::EventLoop *event_loop,
        const std::vector<::cube::net::InetAddr> &addrs,
        const std::string &passwd)
    : m_event_loop(event_loop),
    m_redis_addrs(addrs),
    m_redis_addr_idx(0),
    m_passwd(passwd) {

    assert(!m_redis_addrs.empty());
}

RedisClient::~RedisClient() {
    HandleClose();
}

const ::cube::net::InetAddr &RedisClient::GetNextAddr() {
    size_t idx = m_redis_addr_idx;
    // round robin
    if (++m_redis_addr_idx >= m_redis_addrs.size())
        m_redis_addr_idx = 0;
    return m_redis_addrs[idx];
}

RedisConnectionPtr RedisClient::GetConn() {
    const ::cube::net::InetAddr &addr = GetNextAddr();
    RedisConnectionPtr conn;
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

    // slow path: create a new one because no idle connections now
    M_LOG_TRACE("create a new redis connection");
    conn = RedisConnection::Connect(m_event_loop, addr);
    if (!conn) {
        // error
        M_LOG_WARN("RedisConnection Connect failed");
        return conn;
    }

    conn->SetDisconnectCallback(std::bind(&RedisClient::OnDisconnect, this, _1));
    conn->Initialize();
    //M_LOG_TRACE("new redis connection initialize succ");

    // auth
    if (m_passwd.length() > 0) {
        int ret = conn->Exec(
                std::bind(&RedisClient::OnStatusReply, this, conn, "OK", _1),
                "AUTH %s", m_passwd.c_str());
        if (ret != CUBE_OK) {
            M_LOG_WARN("conn[%lu] issues auth command failed", conn->Id());
            conn->Close();
            conn.reset();
        }
    }

    return conn;
}

void RedisClient::PutConn(RedisConnectionPtr conn) {
    auto &idle_list = m_idle_conns[conn->PeerAddr().IpPort()];
    // at most 16 idle conns per [Ip:Port]
    if (idle_list.size() < 16) {
        idle_list.insert(std::make_pair(conn->Id(), std::move(conn)));
    } else {
        conn->Close();
    }
}

int RedisClient::Exec(const RedisReplyCallback &callback,
        int64_t timeout_ms, const char *format, ...) {

    va_list ap;
    va_start(ap, format);
    int status = Exec(callback, timeout_ms, format, ap);
    va_end(ap);
    return status;
}

int RedisClient::Exec(const RedisReplyCallback &callback,
        int64_t timeout_ms, const char *format, va_list ap) {

    RedisConnectionPtr conn = GetConn();
    if (!conn) {
        // no idle connections and connect failed??
        M_LOG_WARN("GetConn failed");
        return CUBE_ERR;
    }

    int status = conn->Exec(
            std::bind(&RedisClient::OnRedisReply, this, callback, conn, _1),
            format, ap);
    if (status != CUBE_OK) {
        // issue command failed, so close the connection
        M_LOG_WARN("conn[%lu] exec command failed", conn->Id());
        conn->Close();
        return status;
    }

    // add timeout timer
    uint64_t timer_id = m_event_loop->RunAfter(
                std::bind(&RedisClient::OnRedisReplyTimeout,
                this, conn), timeout_ms);
    m_timer_ids[conn->Id()] = timer_id;

    M_LOG_DEBUG("Exec successfully");
    return CUBE_OK;
}

int RedisClient::Exec(const RedisReplyCallback &callback,
        int64_t timeout_ms, int argc,
        const char **argv, const size_t *argvlen) {

    RedisConnectionPtr conn = GetConn();
    if (!conn) {
        // no idle connections and connect failed??
        M_LOG_WARN("GetConn failed");
        return CUBE_ERR;
    }

    int status = conn->Exec(
            std::bind(&RedisClient::OnRedisReply, this, callback, conn, _1),
            argc, argv, argvlen);
    if (status != CUBE_OK) {
        // issue command failed, so close the connection
        M_LOG_WARN("conn[%lu] exec command failed", conn->Id());
        conn->Close();
        return status;
    }

    // add timeout timer
    uint64_t timer_id = m_event_loop->RunAfter(
            std::bind(&RedisClient::OnRedisReplyTimeout,
                this, conn), timeout_ms);
    m_timer_ids[conn->Id()] = timer_id;

    M_LOG_DEBUG("Exec successfully");
    return CUBE_OK;
}

int RedisClient::Exec(const RedisReplyCallback &callback,
        int64_t timeout_ms, const RedisCommand &cmd) {

    M_LOG_TRACE("");

    const std::vector<std::string> &args = cmd.m_args;
    std::vector<const char *> argv;
    std::vector<size_t> argvlen;
    for (auto it = args.begin(); it != args.end(); it++) {
        argv.push_back(it->c_str());
        argvlen.push_back(it->length());
    }

    return Exec(callback, timeout_ms,
            (int)argv.size(), argv.data(), argvlen.data());
}

int RedisClient::MultiExec(const RedisReplyCallback &callback,
        int64_t timeout_ms, const RedisCommands &cmds) {

    M_LOG_TRACE("");
    
    RedisConnectionPtr conn = GetConn();
    if (!conn) {
        // no idle connections and connect failed??
        M_LOG_WARN("GetConn failed");
        return CUBE_ERR;
    }
 
    // MULTI
    int status = CUBE_OK;
    status = conn->Exec(
            std::bind(&RedisClient::OnStatusReply, this, conn, "OK", _1),
            "MULTI");
    if (status != CUBE_OK) {
        // issue command failed, so close the connection
        M_LOG_WARN("conn[%lu] exec failed", conn->Id());
        conn->Close();
        return status;
    }

    // CMDS
    std::vector<const char *> argv;
    std::vector<size_t> argvlen;
    for (auto it = cmds.m_cmds.begin(); it != cmds.m_cmds.end(); it++) {
        const std::vector<std::string> &args = it->m_args;
        argv.resize(args.size());
        argvlen.resize(args.size());
        for (size_t i = 0; i < args.size(); i++) {
            argv[i] = args[i].c_str();
            argvlen[i] = args[i].length();
        }
        status = conn->Exec(
                std::bind(&RedisClient::OnStatusReply, this, conn, "QUEUED", _1),
                (int)argv.size(), argv.data(), argvlen.data());
        if (status != CUBE_OK) {
            // issue command failed, so close the connection
            M_LOG_WARN("conn[%lu] exec failed", conn->Id());
            conn->Close();
            return status;
        }
    }

    // EXEC
    status = conn->Exec(
            std::bind(&RedisClient::OnRedisReply, this, callback, conn, _1),
            "EXEC");
    if (status != CUBE_OK) {
        // issue command failed, so close the connection
        M_LOG_WARN("conn[%lu] exec failed", conn->Id());
        conn->Close();
        return status;
    }

    // add timeout timer
    uint64_t timer_id = m_event_loop->RunAfter(std::bind(&RedisClient::OnRedisReplyTimeout,
                this, conn), timeout_ms);
    m_timer_ids[conn->Id()] = timer_id;

    return CUBE_OK;
}

void RedisClient::OnDisconnect(RedisConnectionPtr conn) {
    auto it = m_idle_conns.find(conn->PeerAddr().IpPort());
    if (it == m_idle_conns.end())
        return;

    M_LOG_TRACE("remove conn[%lu] from idle list", conn->Id());

    it->second.erase(conn->Id());

    // remove empty map
    if (it->second.empty())
        m_idle_conns.erase(it);
}

void RedisClient::OnRedisReply(RedisReplyCallback callback, RedisConnectionPtr conn, redisReply *reply) {
    // remove timeout timer 
    auto it = m_timer_ids.find(conn->Id());
    if (it != m_timer_ids.end()) {
        m_event_loop->CancelTimer(it->second);
        m_timer_ids.erase(it);
    }

    // callback
    callback(reply);

    // conn has been closed
    if (conn->Closed()) 
        return;

    if (reply == NULL || reply->type == REDIS_REPLY_ERROR) {
        conn->Close();
        return;
    }

    // put conn into idle list
    PutConn(conn);
}

void RedisClient::OnRedisReplyTimeout(RedisConnectionPtr conn) {
    
    M_LOG_WARN("OnRedisReplyTimeout conn[%lu] local_addr[%s], peer_addr[%s]",
            conn->Id(),
            conn->LocalAddr().IpPort().c_str(),
            conn->PeerAddr().IpPort().c_str());
    
    // we know timeout-timer has been activated
    // so remove from timer_ids
    m_timer_ids.erase(conn->Id());

    // call back by calling redisAsyncFree in Close()
    conn->Close();
}

void RedisClient::OnStatusReply(RedisConnectionPtr conn,
        const char *status, redisReply *reply) {
    if (reply == NULL) {
        M_LOG_WARN("conn[%lu] reply == NULL, expected status[%s]",
                conn->Id(), status);
        conn->Close();
        return;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        M_LOG_WARN("conn[%lu] reply error=%s, expected status[%s]",
                conn->Id(), reply->str, status);
        conn->Close();
        return;
    }
    
    if (reply->type != REDIS_REPLY_STATUS || strcmp(reply->str, status) != 0) {
        M_LOG_TRACE("conn[%lu] reply type[%d], expected status[%s]",
                conn->Id(), reply->type, status);
        conn->Close();
        return;
    }

    M_LOG_TRACE("conn[%lu] reply expected status[%s]",
            conn->Id(), reply->str);
}

void RedisClient::HandleClose() {
    // close all connections
    auto idle_conns = std::move(m_idle_conns);
    for (auto it = idle_conns.begin(); it != idle_conns.end(); it++) {
        for (auto conn_it = it->second.begin(); conn_it != it->second.end(); conn_it++) {
            conn_it->second->Close();
        }
    }
}

}

}
