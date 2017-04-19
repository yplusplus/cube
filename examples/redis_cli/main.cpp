#include <iostream>
#include <functional>
#include <signal.h>

#include "hiredis.h"
#include "async.h"

#include "net/inet_addr.h"
#include "net/event_loop.h"
#include "net/redis/redis_client.h"

using namespace std;
using namespace cube;
using namespace cube::redis;
using namespace std::placeholders;

EventLoop g_event_loop;
InetAddr g_server_addr("127.0.0.1", 6379);
RedisClient g_redis_client(&g_event_loop);
int g_reply_count = 0;

void HandleSignal(int sig) {
    cout << "signal = " << sig << endl;
    g_event_loop.Stop();
}

void OnRedisReply(redisReply *reply) {
    cout << "OnRedisReply" << endl;
    if (reply)
        cout << reply->str << endl;
    else 
        cout << "reply is NULL" << endl;
    if (++g_reply_count == 2) {
        g_event_loop.Stop();
    } else {
        g_redis_client.IssueCommand(
                g_server_addr, std::bind(OnRedisReply, _1), 10000, "GET cube");
    }
}

int main() {
    signal(SIGINT, HandleSignal);
    const char *argv[3] = {"SET", "cube", "cUbE9"};
    const size_t argvlen[3] = {3, 4, 5};
    g_redis_client.IssueCommand(g_server_addr, std::bind(OnRedisReply, _1),
            10000, 3, &argv[0], argvlen);
    g_event_loop.Loop();
    return 0;
}
