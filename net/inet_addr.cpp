#include <assert.h>
#include <string.h>

#include "inet_addr.h"

/* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };
//
//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };
//
//     struct sockaddr_in6 {
//         sa_family_t     sin6_family;   /* address family: AF_INET6 */
//         uint16_t        sin6_port;     /* port in network byte order */
//         uint32_t        sin6_flowinfo; /* IPv6 flow information */
//         struct in6_addr sin6_addr;     /* IPv6 address */
//         uint32_t        sin6_scope_id; /* IPv6 scope-id */
//     };

namespace cube {

InetAddr::InetAddr(const struct sockaddr_in &addr) 
    : m_addr(addr) {
}

InetAddr::InetAddr(const InetAddr &addr) 
    : m_addr(addr.m_addr) { 
}

InetAddr::InetAddr(const std::string &ip, uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    inet_aton(ip.c_str(), &m_addr.sin_addr);
}

InetAddr::InetAddr(uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = htonl(INADDR_ANY);
}

std::string InetAddr::Ip() const {
    char *buf = inet_ntoa(m_addr.sin_addr);
    assert(buf != NULL);
    return buf;
}

std::string InetAddr::IpPort() const {
    return Ip() + ":" + std::to_string(static_cast<unsigned long long>(Port()));
}

uint16_t InetAddr::Port() const {
    uint16_t port = ntohs(m_addr.sin_port);
    return port;
}

uint16_t InetAddr::NetworkOrderPort() const {
    return m_addr.sin_port;
}

}
