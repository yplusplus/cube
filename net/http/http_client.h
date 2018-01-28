#ifndef __CUBE_HTTP_CLIENT_H__
#define __CUBE_HTTP_CLIENT_H__

#include <string>
#include <vector>
#include <map>

#include "net/tcp_connection.h"
#include "http_cli_connection.h"
#include "http_request.h"

namespace cube {

namespace http {

class HTTPClient {
    public:
        static const size_t DEFAULT_MAX_IDLE_CONNS_NUMBER = 16;

        typedef std::function<void(const HTTPResponse *)> ResponseCallback;

        HTTPClient(::cube::net::EventLoop *event_loop);
        ~HTTPClient();

        // executes a http request
        int Send(const ::cube::net::InetAddr &addr,
                const HTTPRequest &request,
                const ResponseCallback &callback,
                int64_t timeout_ms = 2000);

        // Get and Post Method only provide simple function,
        // use Send for more features
        int Get(const ::cube::net::InetAddr &addr,
                const std::string &url,
                const ResponseCallback &callback,
                int64_t timeout_ms = 2000);

        int Post(const ::cube::net::InetAddr &addr,
                const std::string &url,
                const std::string &body,
                const ResponseCallback &callback,
                int64_t timeout_ms = 2000);

 
    private:
        HTTPClientConnectionPtr GetConn(const ::cube::net::InetAddr &addr);
        void PutConn(HTTPClientConnectionPtr conn);

        void OnClose(::cube::net::TcpConnectionPtr conn);

        void OnResponse(const ResponseCallback &callback,
                HTTPClientConnectionPtr conn, const HTTPResponse *response);
        void OnResponseTimeout(HTTPClientConnectionPtr conn);

    private:
        ::cube::net::EventLoop *m_event_loop;

        typedef std::map<uint64_t, HTTPClientConnectionPtr> ConnsMap;
        std::map<std::string, ConnsMap> m_idle_conns;
        ConnsMap m_requesting_conns;

        std::map<uint64_t, uint64_t> m_timeouts;
};

}

}

#endif
