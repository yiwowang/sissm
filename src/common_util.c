#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsd.h"
#include "log.h"
#include "events.h"
#include "cfs.h"
#include "util.h"
#include "alarm.h"
#include "p2p.h"

#include "roster.h"
#include "api.h"
#include "sissm.h"

#include <ctype.h>
#include <http.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <sys/io.h>
#endif

#include "time.h"

#include "common_util.h"
int isDebug() {
	return 1;
}
int isValidUid(char* uid) {
	return strlen(uid) > 10;
}
void getTimeStr(long t, char* formatYMDHMS, char* output, int outputSize) {
	time_t tt = t > 0 ? (time_t)t : time(NULL);

	struct tm* timeP;
	timeP = localtime(&tt);    // 转换
	snprintf(output, outputSize, formatYMDHMS, 1900 + timeP->tm_year, 1 + timeP->tm_mon, timeP->tm_mday, timeP->tm_hour, timeP->tm_min, timeP->tm_sec);
}

int exists(const char* path)
{
	if (strlen(path) <=0) {
		return 0;
	}
#ifdef _WIN32
	return (_access(path, 0) == 0);
#else
 return (access(path, 0) == 0);
#endif
}

void writeFile(char* path, char* str) {
	FILE* fp = NULL;
	fp = fopen(path, "wb");
	if (fp != NULL)
	{
		fputs(str, fp);
		fclose(fp);
		fp = NULL;
	}
}

void split(const char* src, const char* pattern, char** outvec, size_t* outsize)
{
	const size_t  pat_len = strlen(pattern);
	char* begin = (char*)src;
	const char* next = begin;
	if ((begin = strstr((const char*)begin, pattern)) != 0x00) {
		unsigned int size = begin - next;
		*outvec = malloc(sizeof(char) * size);
		memcpy(*outvec, next, size);
		outvec++;
		(*outsize) += 1;
		split(begin + pat_len, pattern, outvec, outsize);
	}
	else {
		unsigned int size = &src[strlen(src) - 1] - next + 1;
		*outvec = malloc(sizeof(char) * size);
		memcpy(*outvec, next, size);
		(*outsize) += 1;
	}
}

void getWordRight(char* input, char* start, char* output)
{
	strclr(output);

	char* w;
	w = strstr(input, start);
	if (w == NULL)
	{
		return;
	}
	strlcpy(output, w + strlen(start), input-w - strlen(start)+1);
}

void getWordLeft(char* input, char* start, char* output)
{
	strclr(output);
	char* w;
	w = strstr(input, start);
	if (w == NULL)
	{
		return;
	}
	strlcpy(output, input, w - input + 1);
}

// output 大小一定足夠大，最好是300
void getWordRightRange(char* input, char* startOffset, char* start, char* end, char* output) {
	strclr(output);
	if (input == NULL) {
		return;
	}
	if (startOffset!=NULL&&strstr(input, startOffset) != NULL) {
		char temp[500];
		getWordRight(input, startOffset, temp);
		getWordRange(temp, start, end, output);
	}
	else {
		getWordRange(input, start, end, output);
	}
}
void getWordRange(char* input, char* start, char* end, char* output)
{
	strclr(output);

	char* w;
	w = strstr(input, start);
	if (w == NULL)
	{
		return;
	}
	char* w2;
	w2 = strstr(w, end);
	if (w2 == NULL)
	{
		return;
	}

	strlcpy(output, w + strlen(start), w2 - w - strlen(start) + 1);
}

void trim(char* str, char* output, int size)
{
	strclr(output);

	if (strlen(str) == 0)
	{
		return;
	}
	char* p = str;
	char* p1;
	if (p)
	{
		p1 = p + strlen(str) - 1;
		while (*p && isspace(*p))
			p++;
		while (p1 > p && isspace(*p1))
			*p1-- = '\0';
	}
	strlcpy(output, p, size);
}

// char * to int
int strtoi(const char* str)
{
    int base = 10;
	int res = 0, t;
	const char* p;
	for (p = str; *p; p++)
	{
		if (isdigit(*p))
		{
			t = *p - '0';
		}
		else if (isupper(*p))
		{
			t = *p - 'A' + 10;
		}
		else
		{
			return -1;
		}
		if (t >= base)
			return -1;
		res *= base;
		res += t;
	}
	return res;
}




int parseKillInfo(char* strIn, struct KillInfo* killInfo)
{
	killInfo->playerIndex = -1;
	char str1[500];
	getWordRight(strIn, "Display: ", str1);
	//用来接收返回数据的数组。这里的数组元素只要设置的比分割后的子字符串个数大就好了。
	char* arr1[10] = { 0 };
	//分割后子字符串的个数
	size_t num1 = 0;
	split(str1, " killed ", arr1, &num1);
	if (num1 == 2) {
		char* arr2[10] = { 0 };
		size_t num2 = 0;
		split(arr1[0], " + ", arr2, &num2);
		for (int i = 0; i < num2; i++) {

			if (arr2[i] != NULL && strlen(arr2[i]) > 0) {
				/*printf("len=%d %s \n", strlen(arr2[i]), arr2[i]);
				printf("====%d\n",i);*/
				killInfo->playerIndex++;

				getWordLeft(arr2[i], "[", killInfo->playerName[killInfo->playerIndex]);
				char uidTemp[50];
				getWordRange(arr2[i], "[", "]", uidTemp);
				getWordLeft(uidTemp, ",", killInfo->playerUid[killInfo->playerIndex]);

				/*	printf("playerName->%s==%d\n", killInfo->playerName[killInfo->playerIndex], killInfo->playerIndex);
					printf("playerUid->%s\n", killInfo->playerUid[killInfo->playerIndex]);*/


			}
		}


		char* arr3[10] = { 0 };
		size_t num3 = 0;
		split(arr1[1], " with ", arr3, &num3);
		if (num3 == 2) {
			getWordLeft(arr3[0], "[", killInfo->deadName);
			char uidTemp[50];
			getWordRange(arr3[0], "[", "]", uidTemp);
			getWordLeft(uidTemp, ",", killInfo->deadUid);

			if (strstr(arr3[1], "_C_") != NULL) {
				getWordLeft(arr3[1], "_C_", killInfo->weaponName);
			}
			else {

				if (strstr(arr3[1], "\n") != NULL) {
					getWordLeft(arr3[1], "\n", killInfo->weaponName);
				}
				else {
					strlcpy(killInfo->weaponName, arr3[1], sizeof(killInfo->weaponName));
				}
			}

			/*printf("deadName->%s\n", killInfo->deadName);
			printf("deadUid->%s\n", killInfo->deadUid);
			printf("weaponName->%s\n", killInfo->weaponName);*/
		}

	}

	return 0;
}
