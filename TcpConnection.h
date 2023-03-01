#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

// 定义了该宏变量，在监听到socket的写事件触发后发送数据，不会立刻发会等到下一轮的监听；不定义则每段数据准备好了就发送
// #define MSG_SEND_AUTO    

/// @brief 负责维护tcp通信过程所需要的对象：evloop、监听的事件、读/写缓冲区、请求/相应
class TcpConnection
{ 
private:  
    std::string m_name;
    EventLoop* m_evLoop;
    Channel* m_channel;
    Buffer* m_RDBuffer;
    Buffer* m_WRBuffer;
    // http
    HttpRequest* m_request;
    HttpResponse* m_response;
public:
    TcpConnection(int fd, EventLoop* evLoop);
    ~TcpConnection();
private:
    /// @brief 读回调函数 
    static int processRead(void* arg);

    /// @brief 写回调函数 
    static int processWrite(void* arg);

    /// @brief 回收内存回调函数 
    static int processDestroy(void* arg);
};
