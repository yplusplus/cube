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

typedef std::function<void(int)> EventsCallback;
typedef std::function<void(int fd)> AcceptCallback;
typedef std::function<void(TcpConnectionPtr)> NewConnectionCallback;
typedef std::function<void(TcpConnectionPtr, int)> ConnectCallback;
typedef std::function<void(TcpConnectionPtr)> DisconnectCallback;
typedef std::function<void(TcpConnectionPtr, ::cube::Buffer *)> ReadCallback;
typedef std::function<void(TcpConnectionPtr)> WriteCompleteCallback;

typedef std::function<void(UdpConnectionPtr, char *, size_t, const InetAddr &)> UdpReadCallback;

}

}
#endif
