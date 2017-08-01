#ifndef __CUBE_CONNECTOR_H__
#define __CUBE_CONNECTOR_H__

#include <memory>

namespace cube {

namespace net {

class InetAddr;
class EventLoop;
class TcpConnection;

class Connector {
    public:
        static int Connect(const InetAddr &server_addr, int &sockfd);
        static std::shared_ptr<TcpConnection> Connect(EventLoop *event_loop, const InetAddr &server_addr);
};

}

}
#endif
