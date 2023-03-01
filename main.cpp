#include "Channel.h"
#include "EventLoop.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "TcpServer.h"

int main(int argc, const char* argv[])
{
    // const char* arr[3] = {"NULL", "10001", "./res"};
    // argv = arr;
    // argc = 3;
    // if (argc < 3)
    // {
    //     printf("./a.out port path\n");
    //     return -1;
    // }
    chdir("./res");
    unsigned short port = atoi("10001");

    // 创建服务器
    TcpServer* server = new TcpServer(port, 4);
    server->run();

    return 0;
}