#pragma once
#include "Buffer.h"
#include "HttpResponse.h"
#include <string>
#include <map>
#include <functional>


enum class ProcessState: char
{
    ParseRequestLine,
    ParseRequestHeader,
    ParseRequestBody,
    ParseRequestDone
};


/// @brief http请求对象
class HttpRequest
{
private:
    std::string m_method;
    std::string m_url;
    std::string m_version;
    std::map<std::string, std::string> m_headers;
    ProcessState m_curState;
public:
    HttpRequest();
    ~HttpRequest();

    /// @brief 重置HttpRequest结构体 
    void reset();

    /// @brief 添加状态头, 需要传入指向堆内存的指针
    /// @param key 指向堆内存的字符串指针
    /// @param value 指向堆内存的字符串指针
    void addHeader(const std::string key, const std::string value);

    /// @brief 根据key获取请求头的value(堆内存地址)
    /// @param request 
    /// @param key 
    /// @return 失败返回NULL
    std::string getHeader(const std::string key);

    /// @brief 解析请求行，将结果通过request传出
    /// @param request 传出参数
    /// @param buffer 传入缓存数据
    /// @return 成功返回true
    bool parseRequestLine(Buffer* buffer);

    /// @brief 解析请求头, 仅处理一行
    /// @param request 
    /// @param buffer 
    /// @return 成功返回true
    bool parseRequestHeader(Buffer* buffer);

    /// @brief 解析+处理Http请求
    /// @param request 
    /// @param buffer 
    /// @return 成功返回true
    bool parseHttpRequest(Buffer* recvBuf, HttpResponse *response, Buffer *sendBuf, int socket);

    /// @brief 处理Http请求（解析完成后的动作，组织response对象数据，但不发送）
    /// @param request 
    /// @return 成功返回true
    bool processHttpRequest(HttpResponse *response);

    /// @brief 将unicode转为char
    /// @param from 
    /// @return 
    std::string decodeMsg(std::string from);

    /// @brief 根据文件名(后缀), 返回文件类型
    /// @param name 
    /// @return 返回Content—Type
    const std::string getFileType(const std::string name);

    /// @brief 发送目录信息
    /// @param dirName 
    /// @param cfd 
    static void sendDir(std::string dirName, Buffer* sendBuf, int cfd);

    /// @brief 发送文件
    /// @param fileName 
    /// @param cfd 
    static void sendFile(std::string fileName, Buffer* sendBuf, int cfd);

    inline void setMethod(std::string method)
    {
        m_method = method;
    }

    inline void setUrl(std::string url)
    {
        m_url = url;
    }

    inline void setVersion(std::string version)
    {
        m_version = version;
    }

    inline ProcessState getState()
    {
        return m_curState;
    }

    inline void setState(ProcessState state)
    {
        m_curState = state;
    }
private:
    /// @brief 分隔请求行，将请求行的参数保存在当前类
    /// @return 新的开始地址
    char* splitRequestLine(const char* start, const char* end, const char* sub, std::function<void(std::string)> func);

    /// @brief 16进制转10进制
    /// @param c 
    static int hexToDec(char c);
};




