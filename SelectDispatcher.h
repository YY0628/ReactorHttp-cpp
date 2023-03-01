#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <sys/select.h>
#include <string>

class SelectDispatcher: public Dispatcher
{
private:
    fd_set m_readSet;
    fd_set m_writeSet;
    const int MAXEVENTS = 1024;
public:
    SelectDispatcher(EventLoop* evLoop);
    ~SelectDispatcher();   
    // 添加
    int add() override;
    // 删除
    int remove() override;
    // 修改
    int modify() override;
    // 事件检测, 默认等待2s
    int dispatch(int timeout = 2000) override;

};



