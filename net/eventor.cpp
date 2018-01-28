#include "base/logging.h"

#include "eventor.h"
#include "event_loop.h"

namespace cube {

namespace net {

Eventor::Eventor(EventLoop *event_loop, int fd)
    : m_event_loop(event_loop),
    m_fd(fd),
    m_events(0),
    m_revents(0) {
}

Eventor::~Eventor() {
}

void Eventor::HandleEvents() {
    // 设置唤醒事件
    // 调用回调函数
    // 该回调函数是一个总入口，在函数内部对不同的事件类型执行不同的逻辑
    uint32_t revents = m_revents;
    m_revents = 0;
    m_events_callback(revents);
}

void Eventor::Update() {
    // 在事件循环中更新自己，反向指针的作用
    m_event_loop->UpdateEvents(this);
}

void Eventor::Remove() {
    m_events = 0;
    // 从事件循环中删除自己，不再监听该socket句柄下的事件
    m_event_loop->RemoveEvents(this);
}

}

}
