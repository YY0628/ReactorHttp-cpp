#pragma once
#include "EventLoop.h"
#include "WorkerThread.h"
#include <vector>


/// @brief 线程池模型
class ThreadPool
{
private:
    // 主线程的反应堆模型，如果没有子线程，那么与客户端通信便加入此。否则放在子线程的反应堆模型内
    EventLoop* m_mainLoop;
    std::vector<WorkerThread*> m_workerThreads;
    bool m_isStart;
    int m_threadNum;
    int m_index;
public:
    /// @brief 
    /// @param mainLoop 传入主线程的反应堆模型
    /// @param count 线程数量
    ThreadPool(EventLoop* mainLoop, int count);
    ~ThreadPool();

    /// @brief 从线程池中取出一个反应堆模型。如果有子线程，则平均地取出子线程的反应堆模型；如果没有子线程，则取出主线程的反应堆模型
    /// @return 取出的子线程的反应堆模型
    EventLoop* takeWorkerEventLoop();

    /// @brief 启动线程池 
    void run();
};

