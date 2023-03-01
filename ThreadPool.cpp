#include "ThreadPool.h"
#include <assert.h>

using namespace std;

ThreadPool::ThreadPool(EventLoop* mainLoop, int count): m_index(0)
{
    m_mainLoop = mainLoop;
    m_workerThreads.clear();
    m_isStart = false;
    m_threadNum = count;
}


ThreadPool::~ThreadPool()
{
    for (auto& x: m_workerThreads)
    {
        if (x != nullptr)
        {
            delete x;
            x = nullptr;
        }
    }
}


void ThreadPool::run()
{
    assert(!m_isStart);
    if (m_mainLoop->getTid() != this_thread::get_id()) 
    {
        exit(0);
    }
    
    m_isStart = true;
    if (m_threadNum > 0)
    {
        for (int i=0; i<m_threadNum; i++)
        {
            WorkerThread* worker = new WorkerThread(i);
            worker->run();
            m_workerThreads.push_back(worker);
        }
    }
}


EventLoop* ThreadPool::takeWorkerEventLoop()
{
    assert(m_isStart);
    if (m_mainLoop->getTid() != this_thread::get_id()) 
    {
        exit(0);
    }

    // 从线程池中取出一个反应堆模型
    EventLoop* loop = m_mainLoop;
    if (m_threadNum > 0)
    {
        loop = m_workerThreads[m_index]->getEvLoop();
        m_index = (++m_index ) % m_threadNum;
    }
    return loop;
}

