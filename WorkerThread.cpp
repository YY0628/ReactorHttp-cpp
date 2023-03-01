#include "WorkerThread.h"

using namespace std;

WorkerThread::WorkerThread(int index)
{
    m_tid = thread::id();
    m_thread = nullptr;
    m_threadName = "SubThread-" + to_string(index);
    m_evLoop = nullptr;
}


WorkerThread::~WorkerThread()
{
    if (m_thread != nullptr)
    {
        delete m_thread;
        m_thread = nullptr;
    }
    if (m_evLoop != nullptr)
    {
        delete m_evLoop;
        m_evLoop = nullptr;
    }
}


void WorkerThread::run()
{
    // 创建子线程
    m_thread = new thread(&WorkerThread::threadRunFunc, this);
    // 等待子线程创建EventLoop完毕
    unique_lock<mutex> locker(m_mutex);
    // 如果函数表达式返回true，则直接返回。否则释放锁，等待唤醒。唤醒后再次判断条件是否满足
    m_cond.wait(locker, [this]() {
        return m_evLoop != nullptr;
    });
}


void WorkerThread::threadRunFunc()
{
    m_mutex.lock();
    // 初始化EventLoop
    m_evLoop = new EventLoop(m_threadName);
    m_mutex.unlock();
    // 唤醒主线程条件变量阻塞
    m_cond.notify_one();
    // 运行EventLoop
    m_evLoop->run();
}

