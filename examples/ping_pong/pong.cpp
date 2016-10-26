#include <map>
#include <iostream>
#include <functional>

#include "net/time_util.h"
#include "net/tcp_server.h"
#include "net/tcp_connection.h"
#include "net/event_loop.h"

using namespace cube;
using namespace std;
using namespace std::placeholders;

EventLoop g_event_loop;
std::map<uint64_t, TcpConnectionPtr> g_conns;
int g_pings = 0;

void OnPing(TcpConnectionPtr, Buffer *);
void OnDisconnect(TcpConnectionPtr);
void OnNewConnection(TcpConnectionPtr);

void OnNewConnection(TcpConnectionPtr conn) {
    conn->ReadBytes(4, std::bind(OnPing, _1, _2));
    conn->SetDisconnectCallback(std::bind(OnDisconnect, _1));
    g_conns[conn->Id()] = std::move(conn);
}

void OnDisconnect(TcpConnectionPtr conn) {
    g_conns.erase(conn->Id());
}

void OnPing(TcpConnectionPtr conn, Buffer *buffer) {
    if (buffer->ReadableBytes() < 4) {
        conn->Close();
        return;
    }
    g_pings++;
    conn->Write("pong");
    buffer->Retrieve(4);
    conn->ReadBytes(4, std::bind(OnPing, _1, _2));
}

void ShowStat() {
    int64_t now_ms = TimeUtil::CurrentTimeMs();
    cout << now_ms << ": " << g_pings << " qps" << endl;
    g_pings = 0;
}

int main() {
    g_event_loop.AddRepeatedTimer(ShowStat, 1000);
    InetAddr addr(8456);
    TcpServer server(&g_event_loop, addr);
    server.SetNewConnectionCallback(std::bind(OnNewConnection, _1));
    g_event_loop.Loop();
    return 0;
}
