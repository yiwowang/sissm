#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include "http.h"


#include <stdio.h>

#ifdef _WIN32
//For Windows
int betriebssystem = 1;
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <ws2def.h>
#pragma comment(lib, "Ws2_32.lib")
#include <windows.h>
#include <io.h>
#include <process.h>
#include <time.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
int betriebssystem = 2;
#endif


int httpRequest()
{

	printf("httpRequest ing\n");
	struct sockaddr_in server;
	int  timeout=5000;
	struct hostent* hp;
	char ip[20] = { 0 };
	char* hostname = "82.156.36.121";
	int sockfd;
	char message[1024];
	int size_recv, total_size = 0;
	int len;
	char slen[32];
	char chunk[512];

	memset(message, 0x00, sizeof(message));

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		printf("could not create socket\n");
		return -1;
	}

	if ((hp = gethostbyname(hostname)) == NULL)
	{
		_close(sockfd);
		return -1;
	}

	strcpy(ip, inet_ntoa(*(struct in_addr*)hp->h_addr_list[0]));
	
	printf("ip=%s\n", ip);

	server.sin_addr.s_addr = inet_addr(ip);
	server.sin_family = AF_INET;
	server.sin_port = htons(80);

	if (connect(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0)
	{
		printf("connect error： %d", errno);
		return 1;
	}

	strcpy(message, "GET / HTTP/1.1\r\n");
	strcat(message, "Host: 82.156.36.121\r\n");
	strcat(message, "Content-Type: text/html\r\n");
	strcat(message, "Content-Length: ");
	len = strlen("/");
	sprintf(slen, "%d", 0);
	strcat(message, slen);
	strcat(message, "\r\n");
	strcat(message, "\r\n");

	printf("%s\n", message);

	int ret = send(sockfd, message, strlen(message),0);
	if (ret < 0) {
		printf("Send failed\n");
		printf("%s\n", message);
		_close(sockfd);
		return -1;
	}


	//setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));
	setsockopt(sockfd,SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(int));
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));
	while (1)
	{

		memset(chunk, 0x00, 512);
		size_recv = recv(sockfd, chunk, 512, 0);
		if (size_recv == -1)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				printf("recv timeout ...\n");
				break;
			}
			else if (errno == EINTR)
			{
				printf("interrupt by signal...\n");
				continue;
			}
			else if (errno == ENOENT)
			{
				printf("recv RST segement...\n");
				break;
			}
			else
			{
				printf("unknown error: %d\n", errno);
				_close(sockfd);
				exit(1);
			}
		}
		else if (size_recv == 0)
		{
			printf("peer closed ...\n");
			break;
		}
		else
		{
			total_size += size_recv;
			printf("%s", chunk);
		}
	}
	printf("aaa %s", chunk);
	//_close(sockfd);
	return 0;
}
