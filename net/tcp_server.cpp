#include "base/logging.h"
#include "tcp_server.h"
#include "acceptor.h"
#include "socket.h"

using namespace std::placeholders;

namespace cube {

namespace net {

TcpServer::TcpServer(EventLoop *event_loop, const InetAddr &server_addr)
    : m_event_loop(event_loop),
    m_server_addr(server_addr) {
}

TcpServer::~TcpServer() {
}

bool TcpServer::Start() {
    // 构造acceptor并开始监听
    m_acceptor.reset(new Acceptor(
                m_event_loop,
                m_server_addr,
                std::bind(&TcpServer::OnAccept, this, _1)));

    bool ret = m_acceptor->Listen();
    if (!ret) {
        m_err_msg = m_acceptor->ErrMsg();
        m_acceptor.reset();
    }
    return ret;
}

void TcpServer::Stop() {
    // 停止accept
    m_acceptor->Stop();
    // 停止事件循环监听
    m_event_loop->Stop();
}

void TcpServer::OnAccept(int sockfd) {
    // 新的客户端连接成功建立后, 执行回调函数m_new_connection_callback

    // 同时，OnAccept函数本身，会作为回调函数注册到acceptor中，当accept
    // 成功返回时被执行

    // 获取客户端和服务端地址信息，构造一个TCP连接对象
    InetAddr local_addr(sockets::GetLocalAddr(sockfd));
    InetAddr peer_addr(sockets::GetPeerAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(
                m_event_loop,
                sockfd,
                local_addr,
                peer_addr));

    // put conn into conns map
    // TODO: thread-safety
    m_conns_map[conn->Id()] = conn;
    M_LOG_INFO("New Connection[%lu] in TcpServer localAddr[%s], peerAddr[%s]",
            conn->Id(), local_addr.IpPort().c_str(), peer_addr.IpPort().c_str());

    conn->SetConnectionCallback(m_connection_callback);
    conn->SetCloseCallback(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));

    // make sure to call OnConnectionEstablished in loop-thread
    conn->GetEventLoop()->Post(std::bind(&TcpConnection::OnConnectionEstablished, conn));
}

void TcpServer::RemoveConnection(TcpConnectionPtr conn) {
    M_LOG_TRACE("Remove Connection[%lu] in TcpServer local_addr[%s], peerAddr[%s]",
            conn->Id(),
            conn->LocalAddr().IpPort().c_str(),
            conn->PeerAddr().IpPort().c_str());

    // TODO: thread-safety
    m_conns_map.erase(conn->Id());
}

}

}
