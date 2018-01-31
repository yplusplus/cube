#ifndef __CUBE_SOCKET_H__
#define __CUBE_SOCKET_H__

namespace cube {

namespace net {

class InetAddr;

// 对unix API的封装，对socket句柄进行操作
// 对socket属性的修改，根据socket句柄获取IPv4地址等等

namespace sockets {

int CreateStreamSocket();
int CreateDgramSocket();
bool Bind(int sockfd, const InetAddr &bind_addr);
int CreateNonBlockStreamSocket();
bool SetNonBlocking(int sockfd, bool on);
bool SetNoDelay(int sockfd, bool on);
bool SetQuickAck(int sockfd, bool on);
bool SetReuseAddr(int sockfd, bool on);
bool SetKeepAlive(int sockfd, bool on);
bool SetRecvBuffSize(int sockfd, int size);
bool SetSendBuffSize(int sockfd, int size);
int GetSocketError(int sockfd, int &saved_errno);

InetAddr GetLocalAddr(int sockfd);
InetAddr GetPeerAddr(int sockfd);

}


// 对socket的封装，包括构造socket，bind，listen，accept等API的封装
// 以及对socket属性的修改
class Socket {
    public:
        Socket(int sockfd);
        ~Socket();

        int Fd() const { return m_sockfd; }

        // for acceptor
        bool BindAndListen(const InetAddr &addr, int backlog);
        int Accept();

        bool SetNonBlocking(bool on) { return sockets::SetNonBlocking(m_sockfd, on); }
        bool SetNoDelay(bool on) { return sockets::SetNoDelay(m_sockfd, on); }
        bool SetQuickAck(bool on) { return sockets::SetQuickAck(m_sockfd, on); }
        bool SetReuseAddr(bool on) { return sockets::SetReuseAddr(m_sockfd, on); }
        bool SetKeepAlive(bool on) { return sockets::SetKeepAlive(m_sockfd, on); }
        bool SetRecvBuffSize(int size) { return sockets::SetRecvBuffSize(m_sockfd, size); }
        bool SetSendBuffSize(int size) { return sockets::SetSendBuffSize(m_sockfd, size); }

    private:
        const int m_sockfd;
};

}

}

#endif
