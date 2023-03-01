#include "PollDispatcher.h"



PollDispatcher::PollDispatcher(EventLoop* evLoop): Dispatcher(evLoop)
{
    m_fds = new struct pollfd[MAXEVENTS];
    m_maxNum = 0;
    for (int i=0; i<MAXEVENTS; i++)
    {
        m_fds[i].fd =-1;
        m_fds[i].events = 0;
        m_fds[i].revents = 0;
    }
    m_name = "poll";
}


PollDispatcher::~PollDispatcher()
{
    if (m_fds != NULL)
    {
        delete[] m_fds;
        m_fds = NULL;
    }
}


int PollDispatcher::add()
{
    // 取出事件
    int event = 0;
    if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
        event |= POLLOUT;
    if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
        event |= POLLIN;

    // 再fds数组中找到一个空位放进去，并考虑更新最大编号
    int i = 0;
    for (; i<MAXEVENTS; i++)
    {
        if (m_fds[i].fd == -1)
        {
            m_fds[i].events = event;
            m_maxNum = i >m_maxNum ? i : m_maxNum; // 更新最大编号
            m_fds[i].fd = m_channel->getSocket();
            break;
        }
    }
    if (i >= MAXEVENTS)     // 如果找到的编号为MAXEVENTS表示没有合适位置
    {
        return -1;
    }
    
    return 0;
}


int PollDispatcher::remove()
{
    int i = 0;
    for (; i<MAXEVENTS; i++)    // TODO 此处的循环结束位置是否需要调整
    {
        if (m_fds[i].fd == m_channel->getSocket())
        {
            m_fds[i].fd = -1;
            m_fds[i].events = 0;
            m_fds[i].revents = 0;
            break;
        }
    }

    if (i >= MAXEVENTS)     // 如果找到的编号为MAXEVENTS表示没有合适位置
        return -1;

    // 通过TcpConnection释放资源
    m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));

    return 0;
}


int PollDispatcher::modify()
{
    // 取出事件
    int event = 0;
    if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
        event |= POLLOUT;
    if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
        event |= POLLIN;

    int i = 0;
    for (; i<MAXEVENTS; i++)
    {
        if (m_fds[i].fd == m_channel->getSocket())
        {
            m_fds[i].events = event;
            break;
        }
    }

    if (i >= MAXEVENTS)     // 如果找到的编号为MAXEVENTS表示没有合适位置
        return -1;

    return 0;
}


int PollDispatcher::dispatch(int timeout)
{
    // 监听文件描述符
    int count = poll(m_fds, m_maxNum+1, timeout);
    if (count == -1)
    {
        perror("poll");
        exit(0);
    }

    // 有文件描述符就绪
    for (int i=0; i<m_maxNum+1; i++)
    {
        if (m_fds[i].fd == -1)
            continue;

        int ev = m_fds[i].revents;
        // POLLERR 对方断开连接   POLLHUP 对方断开连接但仍向对方发送数据
        if (ev & POLLERR || ev & POLLHUP)
        {
            // 对方断开了连接，需要删除fd

        }
        // 根据不同事件处理
        if (ev & POLLIN)
        {
            // 读事件
            m_evLoop->eventActivate(m_fds[i].fd, (int)FDEvent::ReadEvent);
        }
        if (ev & POLLOUT)
        {
            // 写事件 
            m_evLoop->eventActivate(m_fds[i].fd, (int)FDEvent::WriteEvent);
        }
    }

    return 0;
}


