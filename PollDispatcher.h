#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <poll.h>
#include <string>

class PollDispatcher: public Dispatcher
{
private:
    int m_maxNum;                 // fds数组中对应最大的有效pollfd编号
    struct pollfd* m_fds;         // struct pollfd数组
    const int MAXEVENTS = 1024;
    std::string m_name = std::string();
public:
    PollDispatcher(EventLoop* evLoop);
    ~PollDispatcher();   
    // 添加
    int add() override;
    // 删除
    int remove() override;
    // 修改
    int modify() override;
    // 事件检测, 默认等待2s
    int dispatch(int timeout = 2000) override;
};



