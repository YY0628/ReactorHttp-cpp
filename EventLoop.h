#pragma once
#include "Dispatcher.h"
#include "Channel.h"
#include <thread>
#include <string>
#include <queue>
#include <mutex>
#include <map>


/// @brief 任务的处理监听文件描述符（事件）方式
enum class ElemType:char {ADD, DELETE, MODIFY};

/// @brief 任务节点，添加/删除/修改 监听文件描述符（事件）
struct ChannelElement
{
    ElemType type;
    Channel* channel;
};

struct Dispatcher;

/// @brief 为反应堆模型，负责多路io复用的配置、fd监听事件的配置、dispatcher所需数据、任务队列
class EventLoop
{
private:
    bool m_isQuit;    // 是否结束
    Dispatcher* m_dispatcher;
    // 任务队列
    std::queue<ChannelElement*> m_taskQ;
    // map
    std::map<int, Channel*> m_channelMap;
    // 线程id, name, mutex
    std::thread::id m_tid;
    std::string m_threadName;
    std::mutex m_mutex; 
    // 用于唤醒阻塞
    int m_socketPair[2];  // socketPair[0] 用于发送数据 socketPair[1] 用于接收数据
public:
    /// @brief 主线程中的EventLoop创建，不带名字
    EventLoop();
    /// @brief 子线程中的EventLoop创建，带名字
    EventLoop(const std::string threadName);

    ~EventLoop();

    /// @brief 启动eventLoop: 1. 监听并处理文件描述符的事件  2. 维护任务队列
    /// @return 
    int run();

    /// @brief 相应事件触发后的执行内容
    /// @param fd 监听的文件描述符
    /// @param event 读/写事件
    /// @return -1表示失败
    int eventActivate(int fd, int event);

    /// @brief 将Channel任务添加到任务队列，主线程：添加监听socket的任务；子线程：添加通信socket的任务。将调用processTaskQ
    /// @param channel 
    /// @param type 
    int addTask(struct Channel* channel, ElemType type);

    /// @brief 维护任务队列(添加、删除、修改)，将使用接下来的几个函数：add, del, mod
    int processTaskQ();

    /// @brief 将Channel 添加到dispatcher(即epoll_wait等)、及map
    /// @param channel 
    /// @return -1表示失败
    int add(struct Channel* channel);

    /// @brief 删除channel fd事件结构体
    /// @param channel 
    /// @return -1表示失败
    int remove(struct Channel* channel);

    /// @brief 修改channel fd事件结构体
    /// @param channel 
    /// @return -1表示失败
    int modify(struct Channel* channel);

    /// @brief 释放Channel所占用的内存
    /// @param channel 
    /// @return 
    int freeChannel(struct Channel* channel);

    /// @brief 读取唤醒socket信息
    int readMessage();

    /// @brief 返回当前eventLoop所在的线程id
    inline std::thread::id getTid()
    {
        return m_tid;
    }

private:
    /// @brief 向socketPair[0] 写数据，用于唤醒阻塞的epoll_wait
    void taskWakeup();
};











