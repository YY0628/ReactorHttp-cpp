#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"


/// @brief 服务器对象
class TcpServer
{
private:
    int m_threadNum;
    EventLoop* m_mainLoop;
    ThreadPool* m_pool;
    int m_lfd;
    unsigned short m_port;
public:
    TcpServer(unsigned short port, int threadNum);
    ~TcpServer();

    /// @brief 初始化监听套接字
    void setListen();

    /// @brief 运行tcp服务器, 创建监听套接字, 并运行
    void run();  

    /// @brief 监听套接字收到连接请求后的回调函数
    int acceptConn();
};


