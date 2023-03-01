#include "TcpServer.h"
#include <arpa/inet.h>
#include <functional>
#include "TcpConnection.h"
#include "Log.h"



TcpServer::TcpServer(unsigned short port, int threadNum)
{
    m_threadNum = threadNum;
    m_mainLoop = new EventLoop();
    m_port = port;
    m_pool = new ThreadPool(m_mainLoop, threadNum);
    setListen();
}


void TcpServer::setListen()
{
    // 1. 创建监听的socket
    m_lfd = socket(AF_INET, SOCK_STREAM, 0);  // arg: ipv4协议、流式传输、TCP
    if (m_lfd == -1)
    {
        perror("socket");
        return ;
    }

    // 2. 设置端口复用
    // 设置调用 close(socket)后, 仍可继续重用该socket。调用close(socket)一般不会立即关闭socket，而经历 TIME_WAIT 的过程
    int opt = 1;
    int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    if (ret == -1)
    {
        perror("setsockopt");
        return ;
    }

    // 3. 绑定端口
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // 当前主机所有ip端口
    //    inet_pton(AF_INET, "192.168.237.131", &addr.sin_addr.s_addr);
    addr.sin_port = htons(m_port);
    ret = bind(m_lfd, (struct sockaddr *)&addr, sizeof addr);
    if (ret == -1)
    {
        perror("bind");
        return ;
    }

    // 4. 设置监听
    // 返回fd
    ret = listen(m_lfd, 128);  // 系统里最大监听数为128，超过还是128
    if (ret == -1)
    {
        perror("listen");
        return ;
    }
}


void TcpServer::run()
{
    Debug("服务器程序已经启动了...");
    // 启动线程池
    m_pool->run();
    // 初始化channel，加入监听套接字任务，并加入线程池
    auto func = std::bind(&TcpServer::acceptConn, this);
    // 主线程监听套接字的任务参数为 TcpServer对象
    Channel* channel = new Channel(m_lfd, FDEvent::ReadEvent, func, nullptr, nullptr, this);
    m_mainLoop->addTask(channel, ElemType::ADD);
    // 启动反应堆模型
    m_mainLoop->run();
}


int TcpServer::acceptConn()
{
    int cfd =  accept(m_lfd, NULL, NULL);
    // 从线程池中取出一个子线程的反应堆实例，取处理这个cfd
    EventLoop* evLoop = m_pool->takeWorkerEventLoop();
    // 将cfd放入 TcpConnection 中处理
    new TcpConnection(cfd, evLoop);
    return 0;
}

