#include <iostream>
#include <functional>

#include "base/time_util.h"
#include "net/tcp_server.h"
#include "net/tcp_connection.h"
#include "net/event_loop.h"

using namespace cube;
using namespace cube::net;
using namespace std;
using namespace std::placeholders;

EventLoop g_event_loop;
int g_pings = 0;

void OnPing(TcpConnectionPtr, Buffer *);
void OnNewConnection(TcpConnectionPtr);

void OnConnection(TcpConnectionPtr conn) {
    if (conn->GetState() == ::cube::net::TcpConnection::ConnState_Connected) {
        conn->ReadBytes(4, std::bind(OnPing, _1, _2));
    }
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
    int64_t now_ms = TimeUtil::CurrentTimeMillis();
    cout << now_ms << ": " << g_pings << " qps" << endl;
    g_pings = 0;
}

int main() {
    g_event_loop.RunPeriodic(ShowStat, 1000);
    InetAddr addr(8456);
    TcpServer server(&g_event_loop, addr);
    server.SetConnectionCallback(std::bind(OnConnection, _1));
    server.Start();
    printf("server start succ, listen addr=%s", addr.IpPort().c_str());
    g_event_loop.Loop();
    return 0;
}
