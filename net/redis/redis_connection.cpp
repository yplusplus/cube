#include <assert.h>

#include "hiredis.h"
#include "async.h"

#include "base/logging.h"

#include "net/event_loop.h"
#include "net/eventor.h"

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
    m_closed(false),
    m_timer_id(0) {

    m_eventor->SetEventsCallback(std::bind(&RedisConnection::HandleEvents, this, _1));
}

RedisConnection::~RedisConnection() {
    // easy to find memory leaks
    assert(m_redis_context == NULL);
    M_LOG_DEBUG("");
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

int RedisConnection::IssueCommand(const RedisReplyCallback &redis_reply_callback, int64_t timeout_ms, const char *format, va_list ap) {
    assert(!m_redis_reply_callback);

    int status = redisvAsyncCommand(m_redis_context, OnRedisReply, NULL, format, ap);
    if (status != REDIS_OK)
        return CUBE_ERR;

    m_redis_reply_callback = redis_reply_callback;
    // add timeout timer
    m_timer_id = m_event_loop->RunAfter(
            std::bind(&RedisConnection::OnRedisReplyTimeout, shared_from_this()), timeout_ms);

    return CUBE_OK;
}

int RedisConnection::IssueCommand(const RedisReplyCallback &redis_reply_callback,
        int64_t timeout_ms, int argc, const char **argv, const size_t *argvlen) {
    assert(!m_redis_reply_callback);

    int status = redisAsyncCommandArgv(m_redis_context, OnRedisReply, NULL, argc, argv, argvlen);
    if (status != REDIS_OK)
        return CUBE_ERR;

    m_redis_reply_callback = redis_reply_callback;
    // add timeout timer
    m_timer_id = m_event_loop->RunAfter(
            std::bind(&RedisConnection::OnRedisReplyTimeout, shared_from_this()), timeout_ms);

    return CUBE_OK;
}

void RedisConnection::Close() {
    HandleClose();
}

void RedisConnection::OnRedisReply(struct redisAsyncContext *redis_context, void *reply, void *privdata) {
    RedisConnection *conn = static_cast<RedisConnection *>(redis_context->ev.data);

    // remove timer if no timeout
    if (conn->m_timer_id > 0) {
        conn->m_event_loop->CancelTimer(conn->m_timer_id);
        conn->m_timer_id = 0; 
    }
        
    assert(conn->m_redis_reply_callback);
    RedisReplyCallback callback = std::move(conn->m_redis_reply_callback);
    conn->m_redis_reply_callback = NULL;
    callback((redisReply *)reply);
}

void RedisConnection::OnRedisReplyTimeout(RedisConnectionPtr conn) {
    
    M_LOG_DEBUG("");
    // time out
    
    conn->m_timer_id = 0;

    // call back throuht call redisAsyncFree in HandleClose()
    conn->HandleClose();
}

void RedisConnection::OnConnect(const redisAsyncContext *redis_context, int status) {
    M_LOG_DEBUG("status=%d", status);

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
    M_LOG_DEBUG("status=%d", status);

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
    M_LOG_DEBUG("");
    assert(m_event_loop->IsLoopThread());
    if (m_redis_context)
        redisAsyncHandleRead(m_redis_context);
}

void RedisConnection::HandleWrite() {
    M_LOG_DEBUG("");
    assert(m_event_loop->IsLoopThread());
    if (m_redis_context)
        redisAsyncHandleWrite(m_redis_context);
}

void RedisConnection::HandleError() {
    assert(m_event_loop->IsLoopThread());

    HandleClose();
}

void RedisConnection::HandleClose() {
    assert(m_event_loop->IsLoopThread());

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
