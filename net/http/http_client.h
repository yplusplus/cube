#ifndef __CUBE_HTTP_CLIENT_H__
#define __CUBE_HTTP_CLIENT_H__

#include <string>
#include <vector>
#include <map>

#include "net/tcp_connection.h"
#include "http_cli_connection.h"

namespace cube {

namespace http {

class HTTPClient {
    public:
        typedef std::function<void(const HTTPResponse *)> ResponseCallback;

        HTTPClient(::cube::net::EventLoop *event_loop);
        ~HTTPClient();

        // executes a http request
        int Send(const ::cube::net::InetAddr &addr,
                const HTTPRequest &request,
                const ResponseCallback &response_callback,
                int64_t timeout_ms = 2000);

    private:
        HTTPClientConnectionPtr GetConn(const ::cube::net::InetAddr &addr);
        void PutConn(HTTPClientConnectionPtr conn);

        void OnDisconnect(HTTPClientConnectionPtr conn);

        void OnResponse(const ResponseCallback &response_callback,
                HTTPClientConnectionPtr conn,
                const HTTPResponse *response);
        void OnResponseTimeout(HTTPClientConnectionPtr conn);

    private:
        ::cube::net::EventLoop *m_event_loop;

        std::map<std::string, std::map<uint64_t, HTTPClientConnectionPtr> > m_idle_conns;
        std::map<uint64_t, HTTPClientConnectionPtr> m_requesting_conns;

        std::map<uint64_t, uint64_t> m_timeouts;
};

}

}

#endif
