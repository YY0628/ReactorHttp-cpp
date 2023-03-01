#include "SelectDispatcher.h"


SelectDispatcher::SelectDispatcher(EventLoop* evLoop): Dispatcher(evLoop)
{
    FD_ZERO(&m_readSet);
    FD_ZERO(&m_writeSet);
}


SelectDispatcher::~SelectDispatcher()
{
}


int SelectDispatcher::add()
{
    if (m_channel->getSocket() >= MAXEVENTS)
        return -1;

    if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
        FD_SET(m_channel->getSocket(), &m_writeSet);
    if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
        FD_SET(m_channel->getSocket(), &m_readSet);

    return 0;
}


int SelectDispatcher::remove()
{
    if (m_channel->getSocket() >= MAXEVENTS)
        return -1;

    if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
        FD_CLR(m_channel->getSocket(), &m_writeSet);
    if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
        FD_CLR(m_channel->getSocket(), &m_readSet);

    // 通过TcpConnection释放资源
    m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));

    return 0;
}


int SelectDispatcher::modify()
{
    if (m_channel->getSocket() >= MAXEVENTS)
        return -1;

    if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
    {
        FD_CLR(m_channel->getSocket(), &m_writeSet);
        FD_SET(m_channel->getSocket(), &m_writeSet);
    }
    if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
    {
        FD_CLR(m_channel->getSocket(), &m_readSet);
        FD_SET(m_channel->getSocket(), &m_readSet);
    }

    return 0;
}


int SelectDispatcher::dispatch(int timeout)
{
    struct timeval val;
    val.tv_sec = timeout * 1000;
    val.tv_usec = 0;
    fd_set rdtmp = m_readSet;
    fd_set wrtmp = m_writeSet;
    // readSet writeSet 为传入传出函数，在传出后，会被修改
    int num = select(MAXEVENTS, &rdtmp, &wrtmp, NULL, &val);
    if (num == -1)
    {
        perror("select");
        exit(0);
    }

    for (int i=0; i<MAXEVENTS; i++)
    {
        if (FD_ISSET(i, &rdtmp))
        {
            // 读文件描述符i 触发
            m_evLoop->eventActivate(i, (int)FDEvent::ReadEvent);
        }
        if (FD_ISSET(i, &wrtmp))
        {
            // 写文件描述符i 触发
            m_evLoop->eventActivate(i, (int)FDEvent::WriteEvent);
        }
    }

    return 0;
}

