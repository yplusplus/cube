#include <iostream>
#include <assert.h>
#include <signal.h>

#include "base/logging.h"
#include "net/event_loop.h"
#include "net/inet_addr.h"
#include "net/http/http_connection.h"
#include "net/http/http_server.h"
#include "net/http/http_client.h"
#include "net/http/http_response.h"

using namespace std;
using namespace std::placeholders;
using namespace cube::net;
using namespace cube::http;

EventLoop g_event_loop;
InetAddr g_server_addr("127.0.0.1", 8456);
int g_count = 0;

void HandleSignal(int sig) {
    cout << "stop" << endl;
    cout << "signal = " << sig << endl;
    g_event_loop.Stop();
}

void OnResponse(const HTTPResponse *response) {
    g_count++;
    if (response) {
        cout << response->ToString() << endl;
        if (g_count == 2) {
            g_event_loop.Stop();
            return;
        }
        //g_client.Send(g_server_addr, g_request, std::bind(OnResponse, _1));
    } else {
        cout << "response is NULL" << endl;
    }
}

int main() {
    signal(SIGINT, HandleSignal);

    HTTPClient g_client(&g_event_loop);
    HTTPRequest g_request;
    g_request.SetURL("/");
    //g_request.SetKeepAlive(true);
    cout << g_request.ToString() << endl;
    g_client.Send(g_server_addr, g_request, std::bind(OnResponse, _1), 10000);
    g_event_loop.RunAfter(std::bind(&::cube::net::EventLoop::Stop, &g_event_loop), 0);
    g_event_loop.Loop();

    return 0;
}
