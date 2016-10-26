#ifndef __CUBE_CALLBACKS_H__
#define __CUBE_CALLBACKS_H__

#include <functional>

namespace cube {

class TcpConnection;
class Buffer;

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef std::function<void(int)> EventsCallback;
typedef std::function<void(int fd)> AcceptCallback;
typedef std::function<void(TcpConnectionPtr conn)> NewConnectionCallback;
typedef std::function<void(TcpConnectionPtr conn, int)> ConnectCallback;
typedef std::function<void(TcpConnectionPtr conn)> DisconnectCallback;
typedef std::function<void(TcpConnectionPtr conn, Buffer *)> ReadCallback;
typedef std::function<void(TcpConnectionPtr conn)> WriteCompleteCallback;

}

#endif
