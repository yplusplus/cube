#ifndef __CUBE_REDIS_CLIENT_H__
#define __CUBE_REDIS_CLIENT_H__

#include <vector>
#include <map>

#include "redis_connection.h"

namespace cube {

namespace net {
class EventLoop;
class InetAddr;
}

namespace redis {

class RedisClient {
    public:
        RedisClient(::cube::net::EventLoop *event_loop);
        ~RedisClient();

        void IssueCommand(const ::cube::net::InetAddr &server_addr,
                const RedisReplyCallback &callback,
                int64_t timeout_ms, const char *format, ...);
        void IssueCommand(const ::cube::net::InetAddr &server_addr,
                const RedisReplyCallback &callback,
                int64_t timeout_ms, const char *format, va_list ap);
        void IssueCommand(const ::cube::net::InetAddr &server_addr,
                const RedisReplyCallback &callback,
                int64_t timeout_ms, int argc,
                const char **argv, const size_t *argvlen);

    private:
        RedisConnectionPtr Connect(const ::cube::net::InetAddr &server_addr);
        RedisConnectionPtr GetConn(const ::cube::net::InetAddr &addr);
        void PutConn(RedisConnectionPtr conn);
        void OnRedisReply(RedisReplyCallback callback, RedisConnectionPtr conn, redisReply *reply);
        void OnDisconnect(RedisConnectionPtr conn);

        void HandleClose();

    private:
        ::cube::net::EventLoop *m_event_loop;

        std::map<std::string, std::vector<RedisConnectionPtr> > m_idle_conns;
        std::map<uint64_t, RedisConnectionPtr> m_requesting_conns;
};

}

}

#endif
