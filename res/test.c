#include <stdlib.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
    char method[8] = {0};
    char path[1024] = {0};
    sscanf("GET /404.html HTTP/1.1", "%[^ ] %[^ ]", method, path);
    
    printf("%s\n", method);
    printf("%s\n", path);

    return 0;
}
