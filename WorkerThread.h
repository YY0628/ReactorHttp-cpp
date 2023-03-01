#pragma once
#include "EventLoop.h"
#include <thread>
#include <string>
#include <mutex>
#include <condition_variable>


/// @brief 线程对象
class WorkerThread
{
private:
    std::thread::id m_tid;           // 线程id
    std::thread* m_thread;           // 线程对象
    std::string m_threadName;        // 线程名
    std::mutex m_mutex;              // 互斥锁
    std::condition_variable m_cond;  // 条件变量
    EventLoop* m_evLoop;        // 反应堆模型
public:
    /// @brief 初始化线程
    /// @param index 线程名的编号
    WorkerThread(int index);
    ~WorkerThread();

    /// @brief 启动线程：创建子线程用于创建evLoop，并阻塞等待完成
    void run();

    /// @brief 返回当前线程的evLoop
    /// @return 
    inline EventLoop* getEvLoop()
    {
        return m_evLoop;
    }
private:
    /// @brief 线程运行函数,：创建evLoop，并解除主线线程阻塞
    void threadRunFunc();
};


