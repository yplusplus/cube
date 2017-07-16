#include <unistd.h> // ::close

#include "event_loop.h"
#include "inet_addr.h"
#include "socket.h"
#include "connector.h"

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

}

}
