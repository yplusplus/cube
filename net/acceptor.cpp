#include "acceptor.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "event_loop.h"
#include "eventor.h"
#include "socket.h"

using namespace std::placeholders;

namespace cube {

Acceptor::Acceptor(EventLoop *event_loop,
	const InetAddr &listen_addr,
    const AcceptCallback &accept_callback)
    : m_event_loop(event_loop),
    m_sock(), m_eventor(),
    m_listen_addr(listen_addr),
    m_accept_callback(accept_callback) {
}

Acceptor::~Acceptor() {
}

bool Acceptor::Listen() {
    // 构造非阻塞socket
    // 设置地址重用
    // bind and listen
    //
    // 重置eventor
    // 向eventor中设置事件唤醒时的回调函数
    // 激活eventor中的读事件
    int sockfd = sockets::CreateNonBlockStreamSocket();
    if (sockfd < 0) {
        m_err_msg = std::string(strerror(errno));
        return false;
    }
    m_sock.reset(new Socket(sockfd));
    m_sock->SetReuseAddr(true);
    if (!m_sock->BindAndListen(m_listen_addr, 1024)) {
        // error
        m_sock.reset();
        return false;
    }
    m_eventor.reset(new Eventor(m_event_loop, sockfd));
    m_eventor->SetEventsCallback(std::bind(&Acceptor::HandleEvents, this, _1));
    m_eventor->EnableReading();
    return true;
}

void Acceptor::Stop() {
    if (m_sock) {
        assert(m_eventor);
        m_eventor->Remove();
        m_eventor.reset();
        m_sock.reset();
    }
}

void Acceptor::HandleEvents(int revents) {
    // acceptor的回调函数，注册到eventor中
    // acceptor只需要监听读事件，所以回调函数中只处理读事件唤醒时的逻辑
    if (revents & Poller::POLLIN) {
        HandleRead();
    }
}

void Acceptor::HandleRead() {
    // accept新到达的客户端连接请求
    // 获得socket文件句柄后调用accept回调函数
    int fd = m_sock->Accept();
    if (fd >= 0) {
        assert(m_accept_callback);
        m_accept_callback(fd);
    } else {
        // error
    }
}

}
