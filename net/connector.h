#ifndef __CUBE_CONNECTOR_H__
#define __CUBE_CONNECTOR_H__

namespace cube {

class InetAddr;

class Connector {
    public:
        static int Connect(const InetAddr &server_addr, int &sockfd);
};

}

#endif
