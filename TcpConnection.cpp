#include "TcpConnection.h"
#include "Log.h"
#include <string>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

TcpConnection::TcpConnection(int fd, EventLoop* evLoop)
{
    m_name = "TcpConnection-" + to_string(fd);
    m_evLoop = evLoop;
    m_RDBuffer = new Buffer(10240);
    m_WRBuffer = new Buffer(10240);
    // http
    m_request = new HttpRequest();
    m_response = new HttpResponse();
    // 读回调函数用于处理客户端发来的请求，写回调函数用于回复客户端， 读写需要用到的数据都在tcpConn对象中
    m_channel = new Channel(fd, FDEvent::ReadEvent, processRead, processWrite, processDestroy, this);
    evLoop->addTask(m_channel, ElemType::ADD);
    // Debug("和客户端建立连接, threadName: %s, threadId: %ld, connName: %s",
    //     m_name.data(), static_cast<unsigned long int>(evLoop->getTid()), m_name.data());
}


TcpConnection::~TcpConnection()
{
    // 注意：无论是读入缓存 还是 写入缓存，读完的标记是可读长度为0。而不是，写缓存的可写空间为0
    if (m_RDBuffer && m_RDBuffer->readableSize() == 0 && m_WRBuffer && m_WRBuffer->readableSize() == 0)
    {
        if (m_RDBuffer != nullptr)
        {
            delete m_RDBuffer;
            m_RDBuffer = nullptr;
        }
        if (m_WRBuffer != nullptr)
        {
            delete m_WRBuffer;
            m_WRBuffer = nullptr;
        }
        if (m_request != nullptr)
        {
            delete m_request;
            m_request = nullptr;
        }
        if (m_response != nullptr)
        {
            delete m_response;
            m_response = nullptr;
        }
        if (m_evLoop != nullptr)
        {
            m_evLoop->freeChannel(m_channel);
        }
    }
    Debug("连接断开, 释放资源, gameover, connName: %s", m_name.data());
}


int TcpConnection::processRead(void* arg)
{
    TcpConnection* conn = static_cast<TcpConnection*>(arg);
    // 接收数据
    int socketFd = conn->m_channel->getSocket();
    int len = conn->m_RDBuffer->socketRead(socketFd);
    Debug("请收到的http请求数据:%s\n %s", conn->m_name.data(), conn->m_RDBuffer->data());
    if (len > 0)
    {
        // 接收到http请求，解析http请求
#ifdef MSG_SEND_AUTO
        conn->m_channel->writeEventEnable(true);
        // 需要等到下一轮epoll_wait 时才会触发， 弊端：无法及时发送，会积压在缓冲区，挤压内容过多会导致内存不够
        conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
#endif
        int flag = conn->m_request->parseHttpRequest(conn->m_RDBuffer, conn->m_response, conn->m_WRBuffer, socketFd);
        if (!flag)
        {
            // 失败，回复一个简单的 404
            const char* errMsg = "HTTP/1.1 400 Bad Request\r\n\r\n";
            conn->m_WRBuffer->appendString(errMsg);
        }
    }

#ifndef MSG_SEND_AUTO    // 断开连接, 不能立刻发送数据，需要等待下一轮epoll_wait，因此不能断开连接
    conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif
    return len;
}


int TcpConnection::processWrite(void* arg)
{
    Debug("开始发送数据，基于写事件发送...");
    TcpConnection* conn = static_cast<TcpConnection*>(arg);
    // 发送数据
    int len = conn->m_WRBuffer->sendData(conn->m_channel->getSocket());
    if (len > 0)
    {
        // 判断数据是否被发送完毕
        if (conn->m_WRBuffer->readableSize() == 0)
        {
            // 1 不再检测写事件
            conn->m_channel->writeEventEnable(false);
            // 2 修改
            conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
            // 3 删除这个节点
            conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
        }
    }
    return len;
}


int TcpConnection::processDestroy(void* arg)
{
    TcpConnection* conn = static_cast<TcpConnection*>(arg);
    if (conn != nullptr)
    {
        delete conn;
        return 0;
    }
    return -1;
}

