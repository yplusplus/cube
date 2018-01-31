#ifndef __CUBE_POLLER_H__
#define __CUBE_POLLER_H__

#include <sys/epoll.h>
#include <vector>
#include <map>

namespace cube {

namespace net {

class EventLoop;
class Eventor;

// poller是对linux epoll的封装
// 事件包括：
//  POLLNONE（无监听事件，同时也是个初始值），
//  POLLIN（读事件），
//  POLLOUT（写事件），
//  POLLERR（错误），
//  POLLHUP（挂断）
//
// 事件管理，包括更新注册的事件，删除事件等
class Poller {
    public:
        static const int EPOLL_EVENT_SIZE = 1024;

        enum {
            POLLNONE = 0,
            POLLIN = EPOLLIN,
            POLLOUT = EPOLLOUT,
            POLLERR = EPOLLERR,
            POLLHUB = EPOLLHUP,
        };

        Poller(EventLoop *event_loop);
        ~Poller();

        bool UpdateEvents(Eventor *eventor);
        bool RemoveEvents(Eventor *eventor);

        void Poll(int timeout_ms, std::vector<Eventor *> &active_eventors);

    private:
        bool EpollOperate(int operation, Eventor *eventor);

    private:
        // 反向指针
        EventLoop *m_event_loop;

        int m_epoll_fd;
        // 用于保存epoll唤醒时保存epoll_event
        struct epoll_event m_epoll_events[EPOLL_EVENT_SIZE];

        // 文件句柄到 Eventor的映射
        std::map<int, Eventor *> m_eventors;
};

}

}

#endif
