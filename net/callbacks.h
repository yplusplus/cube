#ifndef __CUBE_CALLBACKS_H__
#define __CUBE_CALLBACKS_H__

#include <functional>

namespace cube {

class Buffer;

namespace net {

class TcpConnection;
class UdpConnection;
class InetAddr;

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef std::shared_ptr<UdpConnection> UdpConnectionPtr;

// for eventor
typedef std::function<void(int)> EventsCallback;

// for acceptor
typedef std::function<void(int fd)> AcceptCallback;

// for tcp connection
typedef std::function<void(TcpConnectionPtr)> ConnectionCallback;
typedef std::function<void(TcpConnectionPtr)> CloseCallback;
typedef std::function<void(TcpConnectionPtr, ::cube::Buffer *)> ReadCallback;
typedef std::function<void(TcpConnectionPtr)> WriteCompleteCallback;

// for udp connection
typedef std::function<void(UdpConnectionPtr, char *, size_t, const InetAddr &)> UdpReadCallback;

}

}
#endif
