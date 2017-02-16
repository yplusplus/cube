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
        EventLoop *m_event_loop;

        std::unique_ptr<Socket> m_sock;
        std::unique_ptr<Eventor> m_eventor;

        InetAddr m_listen_addr;

        AcceptCallback m_accept_callback;
        std::string m_err_msg;
};

}
#endif
