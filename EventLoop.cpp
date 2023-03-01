#include "EventLoop.h"
#include "EPollDispatcher.h"
#include "PollDispatcher.h"
#include "SelectDispatcher.h"
#include <sys/socket.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

using namespace std;


EventLoop::EventLoop(): EventLoop(string())
{
}


EventLoop::EventLoop(const string threadName)
{
    m_isQuit = true;    // 默认没有启动
    m_dispatcher = new EPollDispatcher(this);
    // m_dispatcher = new PollDispatcher(this);
    // m_dispatcher = new SelectDispatcher(this);

    m_channelMap.clear();

    m_tid = this_thread::get_id();
    m_threadName = threadName==string() ? "MainThread": threadName;

    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
    if (ret == -1)
    {
        perror("socketpair");
        exit(0);
    }

    // socketPair[0] 用于发送数据 socketPair[1] 用于接收数据
    auto fun = bind(&EventLoop::readMessage, this);    // 这么绑定后：传入一个参数this
    Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, fun, nullptr, nullptr, this);
    // 添加到任务队列
    addTask(channel, ElemType::ADD);
}


// 不需要实现，在程序运行的过程中 这个EventLoop必定存在
EventLoop::~EventLoop()
{
}


int EventLoop::run()
{
    m_isQuit = false;
    // 比较线程ID是否正常
    if (m_tid != this_thread::get_id())
        return -1;
    // 循环处理事件
    while (!m_isQuit)
    {
        // 监听并处理文件描述符的事件
        m_dispatcher->dispatch();     // 传入不同的dispatcher 会调用不同函数，体现多态
        // 维护任务队列（添加、删除、修改任务）, 当前解除阻塞会会到这步
        processTaskQ();
    }
    return 0;
}


int EventLoop::eventActivate(int fd, int event)
{
    if (fd < 0)
        return -1;
    
    // 取出channel
    struct Channel* channel = m_channelMap[fd];
    assert(channel->getSocket() == fd);

    if ((event & (int)FDEvent::ReadEvent) && channel->readCallback)
    {
        channel->readCallback(const_cast<void*>(channel->getArg()));
    }
    if ((event & (int)FDEvent::WriteEvent) && channel->writeCallback)
    {
        channel->writeCallback(const_cast<void*>(channel->getArg()));
    }

    return 0;
}


int EventLoop::addTask(struct Channel* channel, ElemType type)
{
    // 创建新节点
    ChannelElement* node = new ChannelElement();
    node->type = type;
    node->channel = channel;
    // 加锁
    m_mutex.lock();
    // 加入队列
    m_taskQ.push(node);
    m_mutex.unlock();

    // 判断执行这个函数的是主线程还是子线程（主线程：添加监听socket的任务；子线程：添加通信socket的任务）
    /*
    * 细节: 
    *   1. 对于链表节点的添加: 可能是当前线程也可能是其他线程(主线程)
    *       1). 修改fd的事件, 当前子线程发起, 当前子线程处理
    *       2). 添加新的fd, 添加任务节点的操作是由主线程发起的
    *   2. 不能让主线程处理任务队列, 需要由当前的子线程取处理
    */
    if (m_tid == this_thread::get_id())
    {
        // 当前子线程（基于子线程的角度分析）
        // 维护任务队列
        processTaskQ();
    }
    else
    {
        // 主线程 -- 告诉子线程处理任务队中的任务
        // 子线程在工作，且被 epoll poll select 阻塞了， 需要唤醒阻塞
        taskWakeup();
    }
    return 0;
}


void EventLoop::taskWakeup()
{
    const char* msg = "Wake Up";
    send(m_socketPair[0], msg, strlen(msg), 0);
}


int EventLoop::processTaskQ()
{
    while (!m_taskQ.empty())
    {
        m_mutex.lock();
        // 取出头节点
        ChannelElement* node = m_taskQ.front();
        m_taskQ.pop();
        m_mutex.unlock();
        Channel* channel = node->channel;
        if (node->type == ElemType::ADD)
        {
            // 添加
            add(channel);
        }
        else if (node->type == ElemType::DELETE)
        {
            // 删除
            remove(channel);
        }
        else if (node->type == ElemType::MODIFY)
        {
            // 修改
            modify(channel);
        }
        delete node;    // 此处销毁的是 ChannelElement 对象内存，其中存储的是channel的指针，channel的实际内容并没有被销毁
        // node = nullptr;
    }
    return 0;
}


int EventLoop::add(struct Channel* channel)
{
    int fd = channel->getSocket();
    // 检查不重复则加入channelMap
    if (m_channelMap.find(fd) == m_channelMap.end())
    {
        m_channelMap.insert(make_pair(fd, channel));
        m_dispatcher->setChannel(channel);
        int ret = m_dispatcher->add();   // 带监听文件描述符加入dispatcher
        return ret;
    }
    return -1;
}


int EventLoop::remove(struct Channel* channel)
{
    int fd = channel->getSocket();
    auto it = m_channelMap.find(fd);
    if (it == m_channelMap.end())
    {
        return -1;
    }
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->remove();
    return ret;
}


int EventLoop::modify(struct Channel* channel)
{
    int fd = channel->getSocket();
    auto it = m_channelMap.find(fd);
    if (it == m_channelMap.end())
    {
        return -1;
    }
    m_channelMap.erase(fd);             // TODO
    m_channelMap.insert(make_pair(fd, channel));
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->modify();
    return ret;
}


int EventLoop::freeChannel(struct Channel* channel)
{
    if (channel == nullptr)
        return -1;
    // 删除 channel 和 fd 对应的关系
    auto it =  m_channelMap.find(channel->getSocket());
    if (it != m_channelMap.end())
    {
        m_channelMap.erase(it);
        close(channel->getSocket());
        delete channel;
        channel = nullptr;
        return 0;
    }
    return -1;
}

int EventLoop::readMessage()
{
    char buf[128];
    int len = recv(m_socketPair[1], buf, sizeof(buf), 0);
    return len;
}











