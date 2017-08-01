#include <unistd.h> // ::close

#include "event_loop.h"
#include "inet_addr.h"
#include "socket.h"
#include "connector.h"
#include "tcp_connection.h"

namespace cube {

namespace net {

int Connector::Connect(const InetAddr &server_addr, int &sockfd) {
    sockfd = sockets::CreateNonBlockStreamSocket();
    if (sockfd < 0) {
        return CUBE_ERR;
    }

    if (::connect(sockfd, server_addr.SockAddr(), sizeof(*server_addr.SockAddr()))) {
        if (errno == EINPROGRESS) {
            // it is ok.
        } else {
            // error
            ::close(sockfd);
            return CUBE_ERR;
        }
    }

    return CUBE_OK;
}

std::shared_ptr<TcpConnection> Connector::Connect(EventLoop *event_loop, const InetAddr &server_addr) {
    int sockfd;
    int ret = Connect(server_addr, sockfd);
    if (ret != CUBE_OK) {
        return NULL;
    }

    return std::make_shared<TcpConnection>(
            event_loop, sockfd,
            sockets::GetLocalAddr(sockfd),
            sockets::GetPeerAddr(sockfd));
}

}

}
