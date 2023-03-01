#include "Channel.h"


Channel::Channel(int fd, FDEvent event, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg)
{
    m_fd = fd;
    m_event = (int)event;
    m_arg = arg;
    readCallback = readFunc;
    writeCallback = writeFunc;
    destroyCallback = destroyFunc;
}


Channel::~Channel()
{}


void Channel::writeEventEnable(bool flag)
{
    if (flag)
    {
        m_event |= static_cast<int>(FDEvent::WriteEvent);
    }
    else
    {
        m_event = m_event & (~(int)FDEvent::WriteEvent);
    }
}


bool Channel::isWriteEventEnable()
{
    return m_event & static_cast<int>(FDEvent::WriteEvent);
}