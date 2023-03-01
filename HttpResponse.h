#pragma once
#include "Buffer.h"
#include <map>
#include <functional>

/// @brief 状态码
enum class StatusCode
{
    Unknown = 0,
    OK = 200,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    BadRequest = 400,
    NotFound = 404
};


/// @brief http响应对象
class HttpResponse
{
private:
    // 状态行：状态码, 状态描述
    StatusCode m_statusCode;
    // 文件名
    std::string m_fileName;
    // 响应头数组map
    std::map<std::string, std::string> m_headers;
    // 状态码及其对应描述
    const std::map<int, std::string> m_info = {
        {0, "Unknown"},
        {200, "OK"},
        {301, "MovedPermanently"},
        {302, "MovedTemporarily"},
        {400, "BadRequest"},
        {404, "NotFound"}
    };
public:
    // 用于发送响应体的回调函数指针
    std::function<void(const std::string, Buffer*, int)> sendDatafunc;
public:
    HttpResponse();
    ~HttpResponse();

    /// @brief 添加http响应头键值对
    void addHeader(const std::string key, const std::string value);

    /// @brief 准备相应数据
    void prepareMsg(Buffer* sendBuf, int socket);
    
    inline void setStatusCode(StatusCode code)
    {
        m_statusCode = code;
    }

    inline void setFileName(std::string fileName)
    {
        m_fileName = fileName;
    }
};


