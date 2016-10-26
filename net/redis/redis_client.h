#ifndef __CUBE_REDIS_CLIENT_H__
#define __CUBE_REDIS_CLIENT_H__

#include <vector>
#include <map>

#include "redis_connection.h"

namespace cube {

class EventLoop;
class InetAddr;

namespace redis {

class RedisClient {
    public:
        RedisClient(EventLoop *event_loop);
        ~RedisClient();

        void IssueCommand(const InetAddr &server_addr,
                const RedisReplyCallback &callback, const char *format, ...);

    private:
        RedisConnectionPtr Connect(const InetAddr &server_addr);
        RedisConnectionPtr GetConn(const InetAddr &addr);
        void PutConn(RedisConnectionPtr conn);
        void OnRedisReply(RedisReplyCallback callback, RedisConnectionPtr conn, redisReply *reply);
        void OnDisconnect(RedisConnectionPtr conn);

        void HandleClose();

    private:
        EventLoop *m_event_loop;

        std::map<std::string, std::vector<RedisConnectionPtr> > m_idle_conns;
        std::map<uint64_t, RedisConnectionPtr> m_requesting_conns;
};

}

}

#endif
