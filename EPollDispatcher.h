#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <sys/epoll.h>
#include <string>

class EPollDispatcher: public Dispatcher
{
private:
    int m_epfd;
    struct epoll_event* m_events;
    const int MAXEVENTS = 1024;

public:
    EPollDispatcher(EventLoop* evLoop);
    ~EPollDispatcher();   
    // 添加
    int add() override;
    // 删除
    int remove() override;
    // 修改
    int modify() override;
    // 事件检测, 默认等待2s
    int dispatch(int timeout = 2000) override;

private:
    /// @brief 将fd即需要监听的事件 加入/删除/修改 eventpoll
    /// @param op EPOLL_CTL_ADD / EPOLL_CTL_DEL / EPOLL_CTL_MOD
    /// @return 失败返回-1
    int epoolctl(int op);
};



