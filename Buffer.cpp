#include "Buffer.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <arpa/inet.h>


Buffer::Buffer(int size): m_capacity(size), m_readPos(0), m_writePos(0)
{
    m_data = (char*) malloc(sizeof(char) * size);   // 使用malloc方便后续使用realloc扩展内存
    memset(m_data, 0, size);
}


Buffer::~Buffer()
{
    if (m_data != nullptr)
    {
        free(m_data);
        m_data = nullptr;
    }
}


void Buffer::resize(int new_size)
{
    int writeAble = writableSize();
    int readAble = readableSize();

    // 剩余空间足够写
    if (writeAble >= new_size)
    {
        return ;
    }
    // 需要整理缓存，讲待读取数据移动到缓存开始位置
    else if (m_readPos + writeAble >= new_size)
    {
        memcpy(m_data, m_data + m_readPos, readAble);
        memset(m_data + readAble, 0, m_capacity - readAble);
        m_readPos = 0;
        m_writePos = readAble;
    }
    // 需要重新分配内存
    else
    {
        char* tmp = static_cast<char*>(realloc(m_data, m_capacity + new_size));
        if (tmp == nullptr)
        {
            return;
        }
        memset(tmp + m_capacity, 0, new_size);
        // 更新数据
        m_data = static_cast<char*>(tmp);
        m_capacity += new_size;
    }
}


int Buffer::appendString(const char* data, int len)
{
    if (m_data == nullptr || len <= 0)
    {
        return -1;
    }
    // 检查扩容
    resize(len);
    // 数据拷贝
    memcpy(m_data + m_writePos, data, sizeof(char) * len);
    m_writePos += len;
    return 0;
}


int Buffer::appendString(const char* data)
{
    int len = strlen(data);
    return appendString(data, len);
}


int Buffer::appendString(const std::string str)
{
    return appendString(str.data());
}


int Buffer::socketRead(int fd)
{
    if (m_data == nullptr)
    {
        return -1;
    }

    // 两个iovec对象，第一个直接放入buffer，如果buffer满了，剩余的放第二个iovec。再将两个iovec合并
    struct iovec vec[2];
    int writeable = writableSize();
    // 先存放在buffer
    vec[0].iov_base = m_data + m_writePos;
    vec[0].iov_len = writeable;
    // buffer满了后再放在新内存
    char* tmp = (char*) malloc(40960);
    vec[1].iov_base = tmp;      // TODO: 参考代码上写的是 buffer->data + buffer->writePos;
    vec[1].iov_len = 40960;
    // 将数据读入struct iovec 数组，前一个iovec满后，读入后一个iovec
    int len = readv(fd, vec, 2);

    if (len == -1)
    {
        return -1;
    }
    // buffer空间足够
    else if (len <= writeable)
    {
        m_writePos = m_writePos + len;
    }
    // buffer空间不足，需要将数据写入扩容后的buffer
    else
    {
        m_writePos = m_capacity;
        appendString(tmp, len-writeable);
    }
    if (tmp != nullptr)
    {
        free(tmp);
        tmp = nullptr;
    }
    
    return len;
}


char* Buffer::findCRLF()
{
    // void *memmem(const void *haystack, size_t haystacklen, 
    //              const void *needle, size_t needlelen);
    char* ptr = (char*) memmem(m_data + m_readPos, readableSize(), "\r\n", 2);;
    return ptr;
}


int Buffer::sendData(int socket)
{
    // 有无数据
    int readable = readableSize();
    if (readable > 0)
    {
        // send第四个参数使用 MSG_NOSIGNAL 。当套接字通信的一端关闭连接后，会阻止内核产生 SIGPIPE信号(管道破裂)
        int len = send(socket, m_data + m_readPos, readable, MSG_NOSIGNAL);
        if (len > 0)
        {
            m_readPos += len;
            usleep(1);
        }
        return len;
    }

    return -1;
}
