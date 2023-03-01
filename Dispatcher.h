#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include <string>
class EventLoop;


/// @brief 分发模型：epoll, poll, select
class Dispatcher
{
protected:
    Channel* m_channel;
    EventLoop* m_evLoop;
    std::string m_name = std::string();
public:
    // 初始化epoll/poll/select 所需数据
    Dispatcher(EventLoop* evLoop): m_evLoop(evLoop) { }
    // 这里使用虚函数是为了防止使用“父类指针指向子类对象”时，自动使用父类的析构函数
    virtual ~Dispatcher() {}
    // 添加
    virtual int add() = 0;
    // 删除
    virtual int remove() = 0;
    // 修改
    virtual int modify() = 0;
    // 事件检测, 默认等待2s
    virtual int dispatch(int timeout = 2000) = 0;
    // 设置待添加的Channel
    inline void setChannel(Channel* channel)
    {
        m_channel = channel;
    }
};



