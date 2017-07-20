#include <assert.h>

#include "async.h"

#include "redis_connection.h"
#include "redis_adapter.h"

namespace cube {

namespace redis {

void RedisCubeAddRead(void *pridata) {
    assert(pridata != NULL);
    RedisConnection *conn = static_cast<RedisConnection *>(pridata);
    conn->EnableReading();
}

void RedisCubeDelRead(void *pridata) {
    assert(pridata != NULL);
    RedisConnection *conn = static_cast<RedisConnection *>(pridata);
    conn->DisableReading();
}

void RedisCubeAddWrite(void *pridata) {
    assert(pridata != NULL);
    RedisConnection *conn = static_cast<RedisConnection *>(pridata);
    conn->EnableWriting();
}

void RedisCubeDelWrite(void *pridata) {
    assert(pridata != NULL);
    RedisConnection *conn = static_cast<RedisConnection *>(pridata);
    conn->DisableWriting();
}

void RedisCubeCleanUp(void *pridata) {
    assert(pridata != NULL);
    RedisConnection *conn = static_cast<RedisConnection *>(pridata);
    conn->Remove();
}

void RedisCubeAttach(redisAsyncContext *ac, RedisConnection *conn) {
    ac->ev.addRead = RedisCubeAddRead;
    ac->ev.delRead = RedisCubeDelRead;
    ac->ev.addWrite = RedisCubeAddWrite;
    ac->ev.delWrite = RedisCubeDelWrite;
    ac->ev.cleanup = RedisCubeCleanUp;
    ac->ev.data = conn;
}

}

}
