#include <assert.h>

#include "hiredis.h"
#include "async.h"

#include "base/logging.h"

#include "net/event_loop.h"
#include "net/eventor.h"
#include "net/socket.h"

#include "redis_connection.h"
#include "redis_adapter.h"

using namespace std::placeholders;

namespace cube {

namespace redis {

uint64_t RedisConnection::m_next_conn_id(1);

RedisConnection::RedisConnection(::cube::net::EventLoop *event_loop,
    redisAsyncContext *redis_context,
    const ::cube::net::InetAddr &local_addr,
    const ::cube::net::InetAddr &peer_addr)
    : m_event_loop(event_loop),
    m_conn_id(__sync_fetch_and_add(&m_next_conn_id, 1)),
    m_redis_context(redis_context),
    m_eventor(new ::cube::net::Eventor(m_event_loop, m_redis_context->c.fd)),
    m_local_addr(local_addr),
    m_peer_addr(peer_addr),
    m_closed(false) {

    m_eventor->SetEventsCallback(std::bind(&RedisConnection::HandleEvents, this, _1));
}

RedisConnection::~RedisConnection() {
    //M_LOG_TRACE("conn[%lu]", m_conn_id);
    // easy to find memory leaks
    assert(m_redis_context == NULL);
}

void RedisConnection::Initialize() {
    RedisCubeAttach(m_redis_context, this);
    redisAsyncSetConnectCallback(m_redis_context, OnConnect);
    redisAsyncSetDisconnectCallback(m_redis_context, OnDisconnect);
    m_eventor->EnableReading();
    m_eventor->EnableWriting();
}

void RedisConnection::EnableReading() {
    m_eventor->EnableReading();
}

void RedisConnection::DisableReading() {
    m_eventor->DisableReading();
}

void RedisConnection::EnableWriting() {
    m_eventor->EnableWriting();
}

void RedisConnection::DisableWriting() {
    m_eventor->DisableWriting();
}

void RedisConnection::Remove() {
    m_eventor->Remove();
}

int RedisConnection::Execv(const RedisReplyCallback &callback,
        const char *format, va_list ap) {

    int status = redisvAsyncCommand(m_redis_context, OnRedisReply, NULL, format, ap);
    if (status != REDIS_OK) {
        M_LOG_WARN("conn[%lu] redisvAsyncCommand failed", Id());
        return CUBE_ERR;
    }

    m_callback_list.push_back(callback);

    return CUBE_OK;
}

int RedisConnection::Exec(const RedisReplyCallback &callback,
        const char *format, ...) {

    va_list ap;
    va_start(ap, format);
    int status = Execv(callback, format, ap);
    va_end(ap);
    return status;
}

int RedisConnection::Exec(const RedisReplyCallback &callback,
        int argc, const char **argv, const size_t *argvlen) {
    M_LOG_TRACE("");

    int status = redisAsyncCommandArgv(m_redis_context, OnRedisReply, NULL, argc, argv, argvlen);
    if (status != REDIS_OK) {
        M_LOG_TRACE("conn[%lu] redisAsyncCommandArgv failed", Id());
        return CUBE_ERR;
    }

    m_callback_list.push_back(callback);

    return CUBE_OK;
}

void RedisConnection::Close() {
    HandleClose();
}

void RedisConnection::OnRedisReply(struct redisAsyncContext *redis_context, void *reply, void *privdata) {
    M_LOG_TRACE("");

    RedisConnection *conn = static_cast<RedisConnection *>(redis_context->ev.data);

    assert(!conn->m_callback_list.empty());
    RedisReplyCallback callback = std::move(conn->m_callback_list.front());
    conn->m_callback_list.pop_front();
    callback((redisReply *)reply);
}

RedisConnectionPtr RedisConnection::Connect(::cube::net::EventLoop *event_loop, const ::cube::net::InetAddr &addr) {
    RedisConnectionPtr conn;
    redisAsyncContext *redis_context = redisAsyncConnect(
            addr.Ip().c_str(), addr.HostOrderPort());
    if (redis_context == NULL) {
        // error
        M_LOG_WARN("redisAsyncConnect failed");
        return conn;
    }

    if (redis_context->err) {
        // error
        M_LOG_WARN("redisAsyncConnect failed");
        redisAsyncFree(redis_context);
        return conn;
    }

    M_LOG_TRACE("redis connect async-connect successfully");

    int sockfd = redis_context->c.fd;
    conn = std::make_shared<RedisConnection>(
            event_loop, redis_context,
            net::sockets::GetLocalAddr(sockfd),
            net::sockets::GetPeerAddr(sockfd));

    return conn;
}

void RedisConnection::OnConnect(const redisAsyncContext *redis_context, int status) {
    M_LOG_TRACE("status=%d", status);

    RedisConnection *conn = static_cast<RedisConnection *>(redis_context->ev.data);

    // prevent redis connection being destroyed in ConnectCallback
    RedisConnectionPtr guard(conn->shared_from_this());

    if (conn->m_connect_callback) {
        conn->m_connect_callback(guard, status == REDIS_OK ? CUBE_OK : CUBE_ERR);
    }

    if (status != REDIS_OK) {
        // we know redisAsyncContext is freeing,
        // set m_redis_context NULL and close the connection.
        conn->m_redis_context = NULL;
        conn->HandleClose();
    }
}

void RedisConnection::OnDisconnect(const redisAsyncContext *redis_context, int status) {
    M_LOG_TRACE("status=%d", status);

    // redisAsyncContext is freeing,
    
    RedisConnection *conn = static_cast<RedisConnection *>(redis_context->ev.data);

    // we know it is not in HandleClose() when m_redis_context is not NULL.
    // so close the redis connection by calling HandleClose().
    if (conn->m_redis_context != NULL) {
        conn->m_redis_context = NULL;
        conn->HandleClose();
    }
}

void RedisConnection::HandleEvents(int revents) {
    // prevent connection being destroyed in HandleXXXX()
    RedisConnectionPtr guard(shared_from_this());

    if (revents & ::cube::net::Poller::POLLERR) {
        HandleError();
    }

    if ((revents & ::cube::net::Poller::POLLHUB) && (revents & ~::cube::net::Poller::POLLIN)) {
        HandleClose();
    }

    if (revents & ::cube::net::Poller::POLLIN) {
        HandleRead();
    }

    if (revents & ::cube::net::Poller::POLLOUT) {
        HandleWrite();
    }
}

void RedisConnection::HandleRead() {
    M_LOG_TRACE("");
    m_event_loop->AssertInLoopThread();
    if (m_redis_context)
        redisAsyncHandleRead(m_redis_context);
}

void RedisConnection::HandleWrite() {
    M_LOG_TRACE("");
    m_event_loop->AssertInLoopThread();
    if (m_redis_context)
        redisAsyncHandleWrite(m_redis_context);
}

void RedisConnection::HandleError() {
    m_event_loop->AssertInLoopThread();

    HandleClose();
}

void RedisConnection::HandleClose() {
    M_LOG_TRACE("");

    m_event_loop->AssertInLoopThread();

    if (Closed()) return;
    m_closed = true;

    if (m_redis_context != NULL) {
        // use redisAsyncFree() instead of redisAsyncDisconnect()
        // because redisAsyncDisconnect() don't release context immediately
        // when there are pending replys in redis context.
        redisAsyncContext *redis_context = m_redis_context;
        m_redis_context = NULL;
        redisAsyncFree(redis_context);
    }

    if (m_disconnect_callback) {
        m_disconnect_callback(shared_from_this());
    }
}

}

}
