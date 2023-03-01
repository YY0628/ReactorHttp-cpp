#include "HttpRequest.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include "Log.h"


using namespace std;


HttpRequest::HttpRequest()
{
    reset();
}


HttpRequest::~HttpRequest()
{
}


void HttpRequest::reset()
{
    m_method = string();
    m_url = string();
    m_version = string();
    m_headers.clear();
    m_curState = ProcessState::ParseRequestLine;
}


void HttpRequest::addHeader(const string key, const string value)
{
    if (key.empty() || value.empty())
        return ;
    m_headers.insert(make_pair(key, value));
}


string HttpRequest::getHeader(const string key)
{
    if (key.empty())
        return string();
    auto it = m_headers.find(key);
    if (it == m_headers.end())
    {
        return string();
    }
    return it->second;
}


char* HttpRequest::splitRequestLine(const char* start, const char* end, const char* sub, function<void(string)> func)
{
    char* space = const_cast<char*>(end);
    if (sub != nullptr)
    {
        int subLen = strlen(sub);
        space = static_cast<char*>(memmem(start, end - start, sub, subLen));
        assert(space != nullptr);
    }
    int length = space - start;
    func(string(start, length));
    return space + 1;
}


bool HttpRequest::parseRequestLine(struct Buffer* readBuf)
{
    // 读出请求行, 保存字符串结束地址
    char* end = readBuf->findCRLF();
    // 保存字符串起始地址
    char* start = readBuf->data();
    // 请求行总长度
    int lineSize = end - start;

    if (lineSize)
    {
        auto methodFunc = bind(&HttpRequest::setMethod, this, placeholders::_1);
        start = splitRequestLine(start, end, " ", methodFunc);
        auto urlFunc = bind(&HttpRequest::setUrl, this, placeholders::_1);
        start = splitRequestLine(start, end, " ", urlFunc);
        auto versionFunc = bind(&HttpRequest::setVersion, this, placeholders::_1);
        splitRequestLine(start, end, nullptr, versionFunc);
        // 为解析请求头做准备
        readBuf->readPosIncrease(lineSize + 2);
        // 修改状态
        setState(ProcessState::ParseRequestHeader);
        return true;
    }
    return false;
}


bool HttpRequest::parseRequestHeader(struct Buffer* readBuf)
{
    char* end = readBuf->findCRLF();
    if (end != nullptr)
    {
        char* start = readBuf->data();
        int lineSize = end - start;
        // 基于: 搜索字符串
        char* middle = static_cast<char*>(memmem(start, lineSize, ": ", 2));
        if (middle != nullptr)
        {
            int keyLen = middle - start;
            int valueLen = end - middle -2;
            if (keyLen > 0 && valueLen > 0)
            {   
                string key(start, keyLen);
                string value(middle + 2, valueLen);
                addHeader(key, value);
            }
            readBuf->readPosIncrease(lineSize + 2);
        }
        else
        {
            // 请求头被解析完了, 跳过空行
            readBuf->readPosIncrease(2);
            // 修改解析状态
            // 忽略 post 请求, 按照 get 请求处理
            m_curState = ProcessState::ParseRequestDone;
        }
        return true;
    }
    return false;
}


bool HttpRequest::parseHttpRequest(Buffer* recvBuf, HttpResponse *response, Buffer *sendBuf, int socket)
{
    // 将recvBuf中的数据读到request对象中 
    bool flag = true;
    while (m_curState != ProcessState::ParseRequestDone)
    {
        switch (m_curState)
        {
        case ProcessState::ParseRequestLine:
            flag = parseRequestLine(recvBuf);
            break;
        case ProcessState::ParseRequestHeader:
            flag = parseRequestHeader(recvBuf);
            break;
        case ProcessState::ParseRequestBody:
            break;
        default:
            break;
        }
        if (!flag)
        {
            return flag;
        }
        // 判断是否解析完毕，如果解析完毕，需要回复数据
        if (m_curState == ProcessState::ParseRequestDone)
        {
            // 1. 解析请求数据， 根据request中的数据处理
            processHttpRequest(response);
            // 2. 组织响应数据并发送
            response->prepareMsg(sendBuf, socket);
        }
    }
    // 还原状态，保证继续处理第二条请求
    m_curState = ProcessState::ParseRequestLine;
    return flag;
}


bool HttpRequest::processHttpRequest(HttpResponse *response)
{
    // 解析http请求行 get /xxx/1.jpg http/1.1\r\n
    // printf("接收到请求: method=%s, path=%s\n", request->method, request->url);
    if (strcasecmp(m_method.data(), "get") != 0)     // 不区分大小写，返回0表示字符串相等
    {
        return false;
    }

    m_url = decodeMsg(m_url);  // unicode转char

    // 处理客户端请求的静态资源（目录或文件）
    const char* file = nullptr;
    if (strcmp(m_url.data(), "/") == 0)     // 返回0表示字符串相等
    {
        file = "./";
    }
    else
    {
        file = m_url.data() + 1;    // 去除首位的 / 
    }

    // 获取文件属性
    struct stat st;
    int ret = stat(file, &st);
    if (ret == -1)
    {
        // 文件不存在， 回复 404
        response->setFileName("404.html");
        response->setStatusCode(StatusCode::NotFound);
        response->addHeader("Content-Type", getFileType(".html"));
        response->sendDatafunc = sendFile;
        return false;
    }

    // 200 OK
    response->setFileName(file);
    response->setStatusCode(StatusCode::OK);

    // 判断文件属性
    if (S_ISDIR(st.st_mode))
    {
        // 把目录内容发送给客户端
        response->addHeader("Content-Type", getFileType(".html"));
        response->sendDatafunc = sendDir;
    }
    else
    {
        // 是文件内容发送给客户端
        // sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
        // sendFile(file, cfd);
        response->addHeader("Content-Type", getFileType(file));
        response->addHeader("Content-Length", to_string(st.st_size));
        response->sendDatafunc = sendFile;
    }

    return true;
}


string HttpRequest::decodeMsg(string msg)
{
    const char* from = msg.data();
    string to = string();
    for ( ; *from != '\0'; ++from)
    {
        //isxdigit 判断字符是不是16进制格式，取值在 0~f
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            // 将第二个、第三个字符转换为 char
            to.append(1, hexToDec(from[1]) * 16 + hexToDec(from[2]));
            from += 2;
        }
        else
        {
            to.append(1, *from);
        }
    }
    to.append(1, '\0');
    return to;
}


int HttpRequest::hexToDec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if ( c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else if ( c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}


const string HttpRequest::getFileType(const string name)
{
    // a.jpg a.mp4 a.html
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char* dot = strrchr(name.data(), '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8";	// 纯文本
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}


/*
<html>
    <head>
        <title>
            <a href="...">xxx</a>
        </title>
    </head>
    <body>
        <tr>
            <td>xxx</td>
        </tr>
    </body>
</html>
*/
void HttpRequest::sendDir(string dirName, Buffer* sendBuf, int cfd)
{
    char buf[4096] = {0};
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName.data());    // html文件前半部

    struct dirent** nameList;   // 作为传出参数, 返回指针数组, 如: struct dirent* nameList[]
    int num = scandir(dirName.data(), &nameList, NULL, alphasort);
    for (int i=0; i<num; i++)
    {
        // 获取文件名
        char* name = nameList[i]->d_name;
        char fullPath[1024] = {0};
        sprintf(fullPath, "%s/%s",dirName.data(), name);
        // 根据不同文件类型处理
        struct stat st;
        stat(fullPath, &st);
        if (S_ISDIR(st.st_mode))    // 目录
        {
            sprintf(buf+strlen(buf), 
                "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>", 
                name, name, st.st_size);
        }
        else    // 文件
        {
            sprintf(buf+strlen(buf), 
                "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>", 
                name, name, st.st_size);
        }

        // send(cfd, buf, strlen(buf), 0);     // 流式传输协议, 报文可以任意分段收/发
        sendBuf->appendString(buf);
#ifndef MSG_SEND_AUTO
        // 发送数据
        sendBuf->sendData(cfd);
#endif
        memset(buf, 0, sizeof(buf));        // 清空缓冲区
        free(nameList[i]);  // 释放函数中创建的内存
    }

    sprintf(buf, "</table></body></html>");    // html文件后半部
    // send(cfd, buf, strlen(buf), 0);
    sendBuf->appendString(buf);
#ifndef MSG_SEND_AUTO
    // 发送数据
    sendBuf->sendData(cfd);
#endif
    free(nameList);
}


void HttpRequest::sendFile(std::string fileName, Buffer* sendBuf, int cfd)
{
    // 1. 打开文件
    int fd = open(fileName.data(), O_RDONLY);
    assert(fd > 0);     // 断言：fd>0 否则程序退出

#if 1
// 此处使用自己写的发送函数
    while (1)
    {
        char buf[1024] = {0};
        int len = read(fd, buf, sizeof buf);
        if (len > 0)
        {
            // int ret = send(cfd, buf, sizeof buf, len);
            sendBuf->appendString(buf, len);
#ifndef MSG_SEND_AUTO
            // 发送数据
            sendBuf->sendData(cfd);
#endif            
        }
        else if (len == 0)  // 发送完毕
        {
            break;
        }
        else
        {
            close(fd);
            perror("read");
        }
    }
# else
// 此处使用senfile发送函数，零拷贝
// sendfile 内部缓冲区有限
    off_t size = lseek(fd, 0, SEEK_END);  // 统计文件指针从0位置到结尾所占的字符数量，即文件大小
    lseek(fd, 0, SEEK_SET);  // 将文件指针移到头部
    off_t off = 0;
    while (off < size)  // sendfile 内部缓冲区有限，因此需要在循环中发送
    {
        // out_fd：待写入内容的文件描述符，一般为accept的返回值
        // in_fd：待读出内容的文件描述符，一般为open的返回值
        // offset：指定从读入文件流的哪个位置开始读，如果为空，则使用读入文件流的默认位置，一般设置为NULL
        //         如果不为NULL，在发送数据前，根据该偏移量开始读文件数据；发送数据之后，更新该偏移量
        // count：两文件描述符之间的字节数，一般给struct stat结构体的一个变量，在struct stat中可以设置文件描述符属性
        int ret = sendfile(cfd, fd, &off, size-off);  // 零拷贝
        // cfd 为非阻塞， fd 为阻塞。 因此在cfd发送数据时，可能还来不及读取fd中的数据。造成ret 返回-1
        usleep(10);
        Debug("ret value: %d\n", ret);
        if (ret == -1 && errno != EAGAIN)   // 产生异常
        {
            perror("sendfile");
            break;
        }
    }
#endif

    close(fd);
}
