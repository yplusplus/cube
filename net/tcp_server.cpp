#include "tcp_server.h"
#include "base/log.h"
#include "event_loop.h"
#include "acceptor.h"
#include "tcp_connection.h"
#include "socket.h"

using namespace std::placeholders;

namespace cube {

TcpServer::TcpServer(EventLoop *event_loop, const InetAddr &server_addr) 
    : m_event_loop(event_loop),
    m_server_addr(server_addr) {
}

TcpServer::~TcpServer() {
}

bool TcpServer::Start() {
    m_acceptor.reset(new Acceptor(
                m_event_loop,
                m_server_addr,
                std::bind(&TcpServer::OnAccept, this, _1)));
    
    bool ret = m_acceptor->Listen();
    if (!ret) {
        m_acceptor.reset();
    }
    return ret;
}

void TcpServer::Stop() {
    // stop the acceptor
    m_acceptor->Stop();

    m_event_loop->Stop();
}

void TcpServer::OnAccept(int sockfd) {
    // new connection
    InetAddr local_addr(sockets::GetLocalAddr(sockfd));
    InetAddr peer_addr(sockets::GetPeerAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(
                m_event_loop,
                sockfd,
                local_addr,
                peer_addr));

    m_event_loop->Post(std::bind(&TcpConnection::Initialize, conn));
    assert(m_new_connection_callback);
    m_new_connection_callback(conn);

    LOG_INFO("New Connection[%lu] in TcpServer localAddr[%s], peerAddr[%s]",
            conn->Id(), local_addr.IpPort().c_str(), peer_addr.IpPort().c_str());
}

}
