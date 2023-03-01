#pragma once
#include <string>


/// @brief 读写数据的缓冲区
class Buffer
{
private:
    // 指向缓冲内存的地址
    char* m_data;
    int m_capacity;
    int m_readPos;
    int m_writePos;
public:
    Buffer(int size);
    ~Buffer();

    /// @brief 检查并扩展缓冲区
    /// @param size 将要写入数据的长度
    void resize(int new_size);

    /// @brief 向缓冲区写入数据
    /// @param data 带写入字符数组 不带 \0
    /// @param len 字符数组长度
    /// @return 失败返回-1
    int appendString(const char* data, int len);

    /// @brief 写入字符串
    /// @param data 带 \0
    /// @return 失败返回-1
    int appendString(const char* data);

    int appendString(const std::string str);

    /// @brief 缓冲区剩余可写容量
    /// @return
    inline int writableSize()
    {
        return m_capacity - m_writePos;
    }

    /// @brief 缓冲区可读容量
    /// @return 
    inline int readableSize()
    {
        return m_writePos - m_readPos;
    }

    /// @brief 从socket读出数据,并写入到缓冲区
    /// @param fd 
    /// @return 读取到的数据长度，失败返回-1
    int socketRead(int fd);

    /// @brief 在Buffer中找到换行标记
    /// @return 失败返回 NULL
    char* findCRLF();

    /// @brief 发送缓冲区数据
    /// @return 发送数据的长度，-1表示失败
    int sendData(int socket);

    /// @brief 得到读数据的起始地址
    inline char* data()
    {
        return m_data + m_readPos;
    }

    /// @brief 将读位置增加count个字节
    /// @return  增加后的新读位置
    inline int readPosIncrease(int count)
    {
        m_readPos += count;
        return m_readPos;
    }
};

