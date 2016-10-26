#ifndef __CUBE_REDIS_ADAPTER_H__
#define __CUBE_REDIS_ADAPTER_H__

struct redisAsyncContext;

namespace cube {

namespace redis {

class RedisConnection;

void RedisCubeAddRead(void *pridata);
void RedisCubeDelRead(void *pridata);
void RedisCubeAddWrite(void *pridata);
void RedisCubeDelWrite(void *pridata);
void RedisCubeCleanUp(void *pridata);

void RedisCubeAttach(redisAsyncContext *ac, RedisConnection *conn);

}

}

#endif
