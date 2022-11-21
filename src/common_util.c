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

#include "common_util.h"

void getWordRight(char* input, char* start, char* output, int size)
{
	char* w;
	w = strstr(input, start);
	if (w == NULL)
	{
		return;
	}

	strlcpy(output, w + strlen(start), size);
}


void getWordRange(char* input, char* start, char* end, char* output)
{
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
int strtoi(const char* str, int base)
{
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
