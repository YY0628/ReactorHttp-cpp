#include "EPollDispatcher.h"
#include <unistd.h>

EPollDispatcher::EPollDispatcher(EventLoop* evLoop): Dispatcher(evLoop)
{
    // 创建epollevent对象，返回文件描述符指代
    m_epfd = epoll_create(1);
    if (m_epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }

    m_events = new struct epoll_event[MAXEVENTS];
    m_name = "epoll";
}


EPollDispatcher::~EPollDispatcher()
{
    if (m_epfd != -1)
    {
        close(m_epfd);
    }
    if (m_events != NULL)
    {
        delete[] m_events;
        m_events = NULL;
    }
}

int EPollDispatcher::epoolctl(int op)
{
    // 取出事件
    if (m_channel == NULL)
        return -1;
    int event = 0;
    if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
        event |= EPOLLOUT;
    if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
        event |= EPOLLIN;
    struct epoll_event epevent;
    epevent.data.fd = m_channel->getSocket();
    epevent.events = event;

    // 配置epoll
    int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &epevent);  // 最后一个参数内容会被解开引用并复制，不需考虑同步互斥

    return ret;    
}

// 添加
int EPollDispatcher::add()
{
    int ret = epoolctl(EPOLL_CTL_ADD); 
    if (ret == -1)
    {
        perror("EPOLL_CTL_ADD");
        exit(0);
    }

    return ret;
}


// 删除
int EPollDispatcher::remove()
{
    int ret = epoolctl(EPOLL_CTL_DEL); 
    if (ret == -1)
    {
        perror("EPOLL_CTL_DEL");
        exit(0);
    }
    // 通过TcpConnection释放资源
    m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));
    return ret;
}


// 修改
int EPollDispatcher::modify()
{
    int ret = epoolctl(EPOLL_CTL_MOD); 
    if (ret == -1)
    {
        perror("EPOLL_CTL_MOD");
        exit(0);
    }
    return ret;
}


// 事件检测, 默认等待2s
// EPOLLERR 对方断开连接
// EPOLLHUP 对方断开连接但仍向对方发送数据
int EPollDispatcher::dispatch(int timeout)
{
    // 监听文件描述符
    int num = epoll_wait(m_epfd, m_events, MAXEVENTS, timeout);
    // 有文件描述符就绪
    for (int i=0; i<num; i++)
    {
        int ev = m_events[i].events;
        int fd = m_events[i].data.fd;
        // EPOLLERR 对方断开连接   EPOLLHUP 对方断开连接但仍向对方发送数据
        if (ev & EPOLLERR || ev & EPOLLHUP)
        {
            // 对方断开了连接，需要删除fd

            continue;
        }
        // 根据不同事件处理
        if (ev & EPOLLIN)
        {
            // 读事件
            m_evLoop->eventActivate(fd, (int)FDEvent::ReadEvent);
        }
        if (ev & EPOLLOUT)
        {
            // 写事件 
            m_evLoop->eventActivate(fd, (int)FDEvent::WriteEvent);
        }
    }

    return 0;
}

