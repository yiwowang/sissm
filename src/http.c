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
void closeSocket(int fd){
#ifdef _WIN32
_close(fd);
#else
close(fd);
#endif

}
int httpRequest(int timeout, int  method, char* hostname, char* pathAndParams, char* postData, char* responseBody)
{

	printf("httpGet ing\n");
	struct sockaddr_in server;
	struct hostent* hp;
	char ip[20] = { 0 };
	int sockfd;
	char message[10*1024];
	int size_recv, total_size = 0;
	int len;
	char slen[32];
	char chunk[2048];

	memset(message, 0x00, sizeof(message));

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		printf("could not create socket\n");
		return -1;
	}

	if ((hp = gethostbyname(hostname)) == NULL)
	{
		closeSocket(sockfd);
		return -1;
	}

	strcpy(ip, inet_ntoa(*(struct in_addr*)hp->h_addr_list[0]));

	server.sin_addr.s_addr = inet_addr(ip);
	server.sin_family = AF_INET;
	server.sin_port = htons(80);

	if (connect(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0)
	{
		printf("connect error： %d", errno);
		return 1;
	}
	char  line1[20];
	if (method == 1) {
		sprintf(line1, "GET %s HTTP/1.1\r\n", pathAndParams);
	}
	else {
		sprintf(line1, "POST %s HTTP/1.1\r\n", pathAndParams);
	}
	strcat(message, line1);


	char  line2[20];
	sprintf(line2, "Host: %s\r\n", hostname);
	strcat(message, line2);

	strcat(message, "Content-Type: text/html\r\n");

	char  line3[20];
	if (method == 1) {
		sprintf(line3, "Content-Length: %d\r\n", 0);
	}
	else {
		sprintf(line3, "Content-Length: %d\r\n", strlen(postData));
	}
	strcat(message, line3);
	strcat(message, "\r\n");
	if (method == 2) {
		strcat(message, postData);
	}

	//printf("sending...\n");
	//printf("%s\n", message);

	int ret = send(sockfd, message, strlen(message), 0);
	if (ret < 0) {
		printf("Send failed\n");
		printf("%s\n", message);
		closeSocket(sockfd);
		return -1;
	}

	//printf("receiving...\n");
	//setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));
	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(int));
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));

	char response[10*1024];
	while (1)
	{

		memset(chunk, 0x00, 2048);
		size_recv = recv(sockfd, chunk, 2048, 0);
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
				closeSocket(sockfd);
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
			strcat(response, chunk);
		}

		if (size_recv < 2048) {
			break;
		}
	}
	//printf("%s\n", response);

	if (NULL != strstr(response, "200 OK")) {
		char* json = strstr(response, "\r\n\r\n");
		if (NULL != json) {
			strcpy(responseBody, json + 4);
		}
		else {
			printf("error format %s\n", response);
		}
	}
	else {
		printf("error %s\n", response);
	}
	// closeSocket(sockfd);
	return 0;
}
