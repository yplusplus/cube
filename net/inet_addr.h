#ifndef __CUBE_INET_ADDR_H__
#define __CUBE_INET_ADDR_H__

#include <arpa/inet.h>
#include <string>

namespace cube {

class InetAddr {
    public:
        InetAddr() {}
        InetAddr(const struct sockaddr_in &addr);
        InetAddr(const InetAddr &addr);
        InetAddr(const std::string &ip, uint16_t port);
        InetAddr(const char *ip, uint16_t port);
        InetAddr(uint32_t ip, uint16_t port);
        InetAddr(uint16_t port);

        // format likes a.b.c.d
        std::string Ip() const;
        // format likes a.b.c.d:port
        std::string IpPort() const;
        // host order
        uint32_t HostOrderIp() const;
        uint16_t HostOrderPort() const;
        uint32_t NetworkOrderIp() const;
        uint16_t NetworkOrderPort() const;

        struct sockaddr *SockAddr() { return (struct sockaddr *)(&m_addr); }
        const struct sockaddr *SockAddr() const { return (const struct sockaddr *)(&m_addr); }
        const struct sockaddr_in &SockAddrIn() const { return m_addr; }

        operator std::string() const { return IpPort(); }

    private:
        struct sockaddr_in m_addr;
};

}


#endif
