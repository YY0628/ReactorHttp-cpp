#pragma once
#include <functional>


// 事件枚举
enum class FDEvent
{
    TimeOut = 0x1,
    ReadEvent = 0x2,
    WriteEvent = 0x4
};

// 定义函数指针
// typedef int (* handleFunc)(void* arg);
// using handleFunc = int (*) (void*);


/// @brief 对于监听fd(事件)的包装: 文件描述符、对应事件、对应回调函数、对应参数
class Channel
{
private:
    // 文件描述符
    int m_fd;
    // 事件, 按位设置，具体见 FDEvent
    int m_event;
    // 读回调函数
    void* m_arg;
public:
    using handleFunc = std::function<int(void*)>;
    // 读回调函数，
    handleFunc readCallback;
    // 写回调函数
    handleFunc writeCallback;
    // 资源销毁函数
    handleFunc destroyCallback;
public:
    // 只能传递全局函数 或 类的静态函数，不能传递非静态函数（由于对象未创建，其成员函数也未创建）
    Channel(int fd, FDEvent event, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg);
    ~Channel();
    
    /// @brief 修改fd的写事件（检测/不检测）
    void writeEventEnable(bool flag);

    /// @brief 判断是否检测写事件
    bool isWriteEventEnable();

    inline int getSocket()
    {
        return m_fd;
    }

    inline int getEvent()
    {
        return m_event;
    }

    inline const void* getArg()
    {
        return m_arg;
    }
};


