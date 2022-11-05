#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <http.h>



#include <stdio.h>


int httpRequest()
{
    struct sockaddr_in server;
    struct timeval timeout = {10, 0};
    struct hostent *hp;
    char ip[20] = {0};
    char *hostname = "www.baidu.com";
    int sockfd;
    char message[1024];
    int size_recv, total_size = 0;
    int len;
    char slen[32];
    char chunk[512];

    memset(message, 0x00, sizeof(message));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        printf("could not create socket\n");
        return -1;
    }

    if((hp=gethostbyname(hostname)) == NULL)
    {
        close(sockfd);
        return -1;
    }

    strcpy(ip, inet_ntoa(*(struct in_addr *)hp->h_addr_list[0]));

    printf("ip=%s\n", ip);

    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(80);

    /*连接服务端*/
    if(connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("connect error： %s", errno);
        return 1;
    }

    /*http协议Get请求*/
    strcpy(message, "GET /?ddd=eee HTTP/1.1\r\n");
    strcat(message, "Host: www.baidu.com\r\n");
    strcat(message, "Content-Type: text/html\r\n");
    strcat(message, "Content-Length: ");
    len = strlen("/?ddd=eee");
    sprintf(slen, "%d", 0);
    strcat(message, slen);
    strcat(message, "\r\n");
    strcat(message, "\r\n");

    printf("%s\n", message);

    /*向服务器发送数据*/
    if(send(sockfd, message, strlen(message) , 0) < 0)
    {
        printf("Send failed\n");
        printf("%s\n", message);
        close(sockfd);
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

    while(1)
    {
        memset(chunk, 0x00, 512);

        /*获取数据*/
        if((size_recv=recv(sockfd, chunk, 512, 0)) == -1)
        {
            if(errno == EWOULDBLOCK || errno == EAGAIN)
            {
                printf("recv timeout ...\n");
                break;
            }
            else if(errno == EINTR)
            {
                printf("interrupt by signal...\n");
                continue;
            }
            else if(errno == ENOENT)
            {
                printf("recv RST segement...\n");
                break;
            }
            else
            {
                printf("unknown error: %d\n", errno);
                close(sockfd);
                exit(1);
            }
        }
        else if(size_recv == 0)
        {
            printf("peer closed ...\n");
            break;
        }
        else
        {
            total_size += size_recv;
            printf("%s" , chunk);
        }
    }

    close(sockfd);

    return 0;
}
