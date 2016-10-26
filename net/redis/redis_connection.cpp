#include <assert.h>

#include "hiredis.h"
#include "async.h"

#include "base/log.h"

#include "net/event_loop.h"
#include "net/eventor.h"

#include "redis_connection.h"
#include "redis_adapter.h"

namespace cube {

namespace redis {

uint64_t RedisConnection::m_next_conn_id(1);

RedisConnection::RedisConnection(EventLoop *event_loop,
    redisAsyncContext *redis_context,
    const InetAddr &local_addr,
    const InetAddr &peer_addr)
    : m_event_loop(event_loop),
    m_conn_id(__sync_fetch_and_add(&m_next_conn_id, 1)),
    m_redis_context(redis_context),
    m_eventor(new Eventor(m_event_loop, m_redis_context->c.fd)),
    m_local_addr(local_addr),
    m_peer_addr(peer_addr),
    m_closed(false) {

    m_eventor->SetReadCallback(
            std::bind(&RedisConnection::HandleRead, this));
    m_eventor->SetWriteCallback(
            std::bind(&RedisConnection::HandleWrite, this));
    m_eventor->SetCloseCallback(
            std::bind(&RedisConnection::HandleClose, this));
    m_eventor->SetErrorCallback(
            std::bind(&RedisConnection::HandleError, this));

}

RedisConnection::~RedisConnection() {
    // easy to find memory leaks
    assert(m_redis_context == NULL);
    LOG_DEBUG("");
}

void RedisConnection::Initialize() {
    RedisCubeAttach(m_redis_context, this);
    redisAsyncSetConnectCallback(m_redis_context, OnConnect);
    redisAsyncSetDisconnectCallback(m_redis_context, OnDisconnect);
    m_eventor->Tie(shared_from_this());
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

int RedisConnection::IssueCommand(const RedisReplyCallback &redis_reply_callback, const char *format, va_list ap) {
    assert(!m_redis_reply_callback);

    int status = redisvAsyncCommand(m_redis_context, OnRedisReply, NULL, format, ap);
    if (status == REDIS_OK) {
        status = CUBE_OK;
        m_redis_reply_callback = redis_reply_callback;
    } else {
        status = CUBE_ERR;
    }
    return status;
}

void RedisConnection::Close() {
    HandleClose();
}

void RedisConnection::OnRedisReply(struct redisAsyncContext *redis_context, void *reply, void *privdata) {
    RedisConnection *conn = static_cast<RedisConnection *>(redis_context->ev.data);
    assert(conn->m_redis_reply_callback);
    RedisReplyCallback callback = std::move(conn->m_redis_reply_callback);
    conn->m_redis_reply_callback = NULL;
    callback((redisReply *)reply);
}

void RedisConnection::OnConnect(const redisAsyncContext *redis_context, int status) {
    LOG_DEBUG("status=%d", status);

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
    LOG_DEBUG("status=%d", status);

    // redisAsyncContext is freeing,
    
    RedisConnection *conn = static_cast<RedisConnection *>(redis_context->ev.data);

    // we know it is not in HandleClose() when m_redis_context is not NULL.
    // so close the redis connection by calling HandleClose().
    if (conn->m_redis_context != NULL) {
        conn->m_redis_context = NULL;
        conn->HandleClose();
    }
}

void RedisConnection::HandleRead() {
    LOG_DEBUG("");
    assert(m_event_loop->IsLoopThread());
    if (m_redis_context)
        redisAsyncHandleRead(m_redis_context);
}

void RedisConnection::HandleWrite() {
    LOG_DEBUG("");
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
