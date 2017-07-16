#ifndef __CUBE_REDIS_CONNECTION_H__
#define __CUBE_REDIS_CONNECTION_H__

#include <stdarg.h>
#include <memory>

#include "net/event_loop.h"
#include "net/inet_addr.h"
#include "net/callbacks.h"

struct redisAsyncContext;
struct redisReply;

namespace cube {

typedef uint64_t TimerId; 

namespace redis {

class RedisConnection;

typedef std::shared_ptr<RedisConnection> RedisConnectionPtr;
typedef std::function<void(redisReply *)> RedisReplyCallback;

class RedisConnection : public std::enable_shared_from_this<RedisConnection> {
public:
    typedef std::function<void(RedisConnectionPtr, int)> ConnectCallback;
    typedef std::function<void(RedisConnectionPtr)> DisconnectCallback;

    RedisConnection(::cube::net::EventLoop *event_loop,
            redisAsyncContext *redis_context,
            const ::cube::net::InetAddr &local_addr,
            const ::cube::net::InetAddr &peer_addr);
    ~RedisConnection();

    void Initialize();

    void EnableReading();
    void DisableReading();
    void EnableWriting();
    void DisableWriting();
    void Remove();

    void SetConnectCallback(const ConnectCallback &cb) { m_connect_callback = cb; }
    void SetDisconnectCallback(const DisconnectCallback &cb) { m_disconnect_callback = cb; }

    int IssueCommand(const RedisReplyCallback &redis_reply_callback, int64_t timeout_ms,
            const char *format, va_list ap);
    int IssueCommand(const RedisReplyCallback &redis_reply_callback, int64_t timeout_ms,
            int argc, const char **argv, const size_t *argvlen);

    const ::cube::net::InetAddr &LocalAddr() const { return m_local_addr; }
    const ::cube::net::InetAddr &PeerAddr() const { return m_peer_addr; }

    uint64_t Id() const { return m_conn_id; }
    void Close();
    bool Closed() const { return m_closed; }

private:
    static void OnRedisReply(struct redisAsyncContext *redis_context, void *reply, void *privdata);
    static void OnRedisReplyTimeout(RedisConnectionPtr conn);
    static void OnConnect(const redisAsyncContext *redis_context, int status);
    static void OnDisconnect(const redisAsyncContext *redis_context, int status);

    void HandleEvents(int revents);
    void HandleRead();
    void HandleWrite();
    void HandleError();
    void HandleClose();

private:
    static uint64_t m_next_conn_id; // atomic

    ::cube::net::EventLoop *m_event_loop;

    const uint64_t m_conn_id;

    redisAsyncContext *m_redis_context;

    std::unique_ptr<::cube::net::Eventor> m_eventor;

    ::cube::net::InetAddr m_local_addr;
    ::cube::net::InetAddr m_peer_addr;

    bool m_closed;

    TimerId m_timer_id;

    ConnectCallback m_connect_callback;
    DisconnectCallback m_disconnect_callback;
    RedisReplyCallback m_redis_reply_callback;
};

}

}

#endif
