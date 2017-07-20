#include <iostream>
#include <functional> 
#include <signal.h>

#include "hiredis.h"
#include "async.h"

#include "base/logging.h"
#include "net/inet_addr.h"
#include "net/event_loop.h"
#include "redis/redis_client.h"

using namespace std;
using namespace cube;
using namespace cube::net;
using namespace cube::redis;
using namespace std::placeholders;

EventLoop g_event_loop;
InetAddr g_server_addr("127.0.0.1", 6379);
std::unique_ptr<RedisClient> g_redis_client(new RedisClient(&g_event_loop, g_server_addr, "123"));
int g_reply_count = 0;

void HandleSignal(int sig) {
    cout << "signal = " << sig << endl;
    g_event_loop.Stop();
}

void print_reply(redisReply *reply) {
    switch (reply->type) {
        case REDIS_REPLY_ERROR:
            cout << "error = " << reply->str << endl;
            return;
        case REDIS_REPLY_STATUS:
            cout << "status = " << reply->str << endl;
            return;
        case REDIS_REPLY_STRING:
            cout << "string = " << reply->str << endl;
            return;
        case REDIS_REPLY_ARRAY:
            {
                cout << "array = {" << endl;
                for (size_t i = 0; i < reply->elements; i++) {
                    print_reply(reply->element[i]);
                }
                cout << "}" << endl;
            }
            return;
        case REDIS_REPLY_INTEGER:
            cout << reply->integer << endl;
            return;
    }
}

void OnRedisReply(redisReply *reply) {
    cout << "OnRedisReply" << endl;
    if (reply == NULL)
        cout << "reply is NULL" << endl;
    else 
        print_reply(reply);
}

int main() {
    ::cube::logging::SetLoggerLevel(::cube::logging::LogLevel_Trace);
    signal(SIGINT, HandleSignal);
    RedisCommands cmds;
    RedisCommand cmd1, cmd2;
    cmd1 << "SET" << "cube" << "cUb\r\nE10";
    cmd2 << "GET" << "cube 1";
    cmds.Add(cmd1)
        .Add(cmd2)
        .Add(RedisCommand() << "LPUSH" << "cube1" << "!@#!@#");
    g_redis_client->Exec(std::bind(OnRedisReply, _1), 1000, cmd2);
    g_redis_client->MultiExec(std::bind(OnRedisReply, _1), 5000, cmds);
    g_event_loop.Loop();
    g_redis_client.reset();
    return 0;
}
