#include <iostream>
#include <assert.h>
#include <signal.h>

#include "net/event_loop.h"
#include "net/inet_addr.h"
#include "net/http/http_connection.h"
#include "net/http/http_server.h"
#include "net/http/http_util.h"

using namespace std;
using namespace std::placeholders;
using namespace cube;
using namespace cube::http;

EventLoop g_event_loop;

void HandleSignal(int sig) {
    cout << "stop" << endl;
    cout << "signal = " << sig << endl;
    g_event_loop.Stop();
}

void HelloHandler(HTTPConnectionPtr conn, const HTTPRequest &request) {
    HTTPResponse response;
    response.SetProto(request.Proto());
    response.SetStatusCode(HTTPStatus_OK);
    response.Write("Hello, World!");
    conn->SendResponse(response);
}

int main() {
    signal(SIGINT, HandleSignal);

    InetAddr server_addr(8456);
    HTTPServer http_server(&g_event_loop, server_addr);
    http_server.SetRequestCallback(std::bind(&HelloHandler, _1, _2));
    assert(http_server.Start());

    g_event_loop.Loop();

    return 0;
}
