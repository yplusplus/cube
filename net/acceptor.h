#ifndef __CUBE_ACCEPTOR_H__
#define __CUBE_ACCEPTOR_H__

#include <memory>

#include "callbacks.h"
#include "inet_addr.h"

namespace cube {

class EventLoop;
class Eventor;
class Socket;

class Acceptor {
    public:
        Acceptor(EventLoop *event_loop,
                const InetAddr &listen_addr,
                const AcceptCallback &accept_callback);
        ~Acceptor();

        bool Listen();
        void Stop();

        const std::string &ErrMsg() const { return m_err_msg; }
    private:
        void HandleEvents(int revents);
        void HandleRead();

    private:
        // 反向指针
        EventLoop *m_event_loop;

        std::unique_ptr<Socket> m_sock;
        std::unique_ptr<Eventor> m_eventor;

        // 监听地址
        InetAddr m_listen_addr;
        // accept成功返回时的回调函数
        AcceptCallback m_accept_callback;
        std::string m_err_msg;
};

}
#endif
