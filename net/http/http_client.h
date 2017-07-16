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

        // TODO set timeout
        void Send(const ::cube::net::InetAddr &addr,
                const HTTPRequest &request, const ResponseCallback &response_callback);

    private:
        HTTPClientConnectionPtr GetConn(const ::cube::net::InetAddr &addr);
        void PutConn(HTTPClientConnectionPtr conn);

        void OnResponse(const ResponseCallback &response_callback,
                std::shared_ptr<HTTPClientConnection> conn, const HTTPResponse *response);

        void OnDisconnect(HTTPClientConnectionPtr conn);

    private:
        ::cube::net::EventLoop *m_event_loop;

        std::map<std::string, std::vector<HTTPClientConnectionPtr> > m_idle_conns;
        std::map<uint64_t, HTTPClientConnectionPtr> m_requesting_conns;
};

}

}

#endif
