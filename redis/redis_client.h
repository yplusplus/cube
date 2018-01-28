#ifndef __CUBE_REDIS_CLIENT_H__
#define __CUBE_REDIS_CLIENT_H__

#include <vector>
#include <map>

#include "redis_connection.h"
#include "redis_command.h"

namespace cube {

namespace net {
class EventLoop;
class InetAddr;
}

namespace redis {

class RedisClient {
    public:
        RedisClient(::cube::net::EventLoop *event_loop,
                const ::cube::net::InetAddr &addr,
                const std::string &passwd = "");
        RedisClient(::cube::net::EventLoop *event_loop,
                const std::vector<::cube::net::InetAddr> &addrs,
                const std::string &passwd = "");
        ~RedisClient();

        int Exec(const RedisReplyCallback &callback,
                int64_t timeout_ms, const char *format, ...);
        int Exec(const RedisReplyCallback &callback,
                int64_t timeout_ms, const char *format, va_list ap);
        int Exec(const RedisReplyCallback &callback,
                int64_t timeout_ms, int argc,
                const char **argv, const size_t *argvlen);
        // recommanded api
        int Exec(const RedisReplyCallback &callback,
                int64_t timeout_ms, const RedisCommand &cmd);
        int MultiExec(const RedisReplyCallback &callback,
                int64_t timeout_ms, const RedisCommands &cmds);

    private:
        const ::cube::net::InetAddr &GetNextAddr();
        RedisConnectionPtr GetConn();
        void PutConn(RedisConnectionPtr conn);

        void OnRedisReply(RedisReplyCallback callback, RedisConnectionPtr conn, redisReply *reply);
        void OnRedisReplyTimeout(RedisConnectionPtr conn);
        void OnDisconnect(RedisConnectionPtr conn);

        // expect to receive a reply with specific status("OK", "QUEUED")
        void OnStatusReply(RedisConnectionPtr conn, const char *status, redisReply *reply);

        void HandleClose();

    private:
        ::cube::net::EventLoop *m_event_loop;

        std::map<std::string, std::map<uint64_t, RedisConnectionPtr> > m_idle_conns;
        std::map<uint64_t, TimerId> m_timer_ids;

        // redis server address
        const std::vector<::cube::net::InetAddr> m_redis_addrs;
        size_t m_redis_addr_idx;

        // auth password
        const std::string m_passwd;
};

}

}

#endif
