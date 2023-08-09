//  ==============================================================================================
//
//  Module: socket_plugin
//
//  Description:
//  Functional example template for plugin developers
//
//  Original Author:
//  Yeye  2019.08.14
//
//  Released under MIT License
//  ID Authenticator: c4c5a1eda6815f65bb2eefd15c5b5058f996add99fa8800831599a7eb5c2a04c
//
//  ==============================================================================================


#include <sys/types.h>
#include <fcntl.h>


#include "cJSON.h"



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
#include "common_util.h"
#include "socket_plugin.h"
// C靠靠靠靠靠靠靠靠靠縮trclr靠靠靠靠靠縭eturn 靠靠靠靠靠靠靠?
//  ==============================================================================================
//  Data definition 
//
#define EVENT_COUNT                    (20)
static struct {

	int  pluginState;                      // always have this in .cfg file:  0=disabled 1=enabled

	char matchEventString[EVENT_COUNT][100];
	int enableLog;

} pluginConfig;

int matchEventMaxIndex = -1;
#define PORT 8006
#define MAX_BUFFER_SIZE 1024
//  ==============================================================================================
//  pluginInitConfig
//
//  Read parameters from the .cfg file 
//
int pluginInitConfig(void)
{

	cfsPtr cP;

	cP = cfsCreate(sissmGetConfigPath());

	// read "plugin.pluginstate" variable from the .cfg file
	pluginConfig.pluginState = (int)cfsFetchNum(cP, "socket_plugin.pluginState", 0.0);  // disabled by default
	pluginConfig.enableLog = (int)cfsFetchNum(cP, "socket_plugin.enableLog", 0.0);  // disabled by default
	logPrintf(LOG_LEVEL_INFO, "plugin", "pluginConfig.pluginState=%d", pluginConfig.pluginState);
	logPrintf(LOG_LEVEL_INFO, "plugin", "pluginConfig.enableLog=%d", pluginConfig.enableLog);
	int i;
	for (i = 0; i < EVENT_COUNT; i++) {
		char key[50];
		snprintf(key, 50, "socket_plugin.matchEventString[%d]", i);
		strclr(pluginConfig.matchEventString[i]);
		strlcpy(pluginConfig.matchEventString[i], cfsFetchStr(cP, key, ""), CFS_FETCH_MAX);
		if (strlen(pluginConfig.matchEventString[i]) > 0) {
			logPrintf(LOG_LEVEL_INFO, "plugin", "socket_plugin.matchEventString[%d] = %s", i, pluginConfig.matchEventString[i]);
			matchEventMaxIndex = i;
		}
	}
	cfsDestroy(cP);
	return 0;
}

#ifndef _WIN32            // Linux
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>
#include "pthread.h" 
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#include <signal.h>
#else                     // Windows
#include <winsock2.h>
#include "winport.h"
#define true  (1)
#define false (0)
#include "windows.h" 
#endif

#define RESULT_DATA_SIZE                    (2000)
#define RESULT_MSG_SIZE                    (1000)
SOCKET new_socket;
SOCKET server_fd;
int aliveSockect = 0;
int socketConnected = 0;
int needReConnect = 0;

#ifdef _WIN32
DWORD WINAPI thread_func(LPVOID lpParam) {
	logPrintf(LOG_LEVEL_INFO, "plugin", "windows new socket thread ok");
	startSocket();
	return 0;
}
#else
#include <pthread.h>
void* thread_func(void* arg)
{
	logPrintf(LOG_LEVEL_INFO, "plugin", "linux new socket thread ok");
	startSocket();
}

#endif


void sendSocket(char* data) {
	if (socketConnected == 0) {
		logPrintf(LOG_LEVEL_INFO, "plugin", "send fail,reason:socketConnected=0");
		return;

	}
	if (strlen(data) == 0) {
		logPrintf(LOG_LEVEL_INFO, "plugin", "send fail,reason:data is empty");
		return;
	}
	char newData[200];
	strcat(data, "\n");
	if (pluginConfig.enableLog == 1) {
		logPrintf(LOG_LEVEL_INFO, "plugin", "send data=%s", data);
	}
	if (send(new_socket, data, strlen(data), 0) < 0) {
		needReConnect = 1;
		logPrintf(LOG_LEVEL_INFO, "plugin", "send faild=%s", data);
	}
}

char* trimStr(char* str)
{
	char* p = str;
	char* p1;
	if (p)
	{
		p1 = p + strlen(str) - 1;
		while (*p && isspace(*p)) p++;
		while (p1 > p && isspace(*p1)) *p1-- = '\0';
	}
	return p;
}

void sendEvent(char* eventType, char* log, char* otherJson) {

	if (otherJson == NULL) {
		otherJson = "";

	}
	char json[1000];

	snprintf(json, 1000, "{\"event_type\":\"%s\",\"log\":\"%s\"%s}", eventType, log, otherJson);
	sendSocket(json);
}



int startThread() {
#ifdef _WIN32

	HANDLE thread;
	DWORD threadId;

	thread = CreateThread(NULL, 0, thread_func, NULL, 0, &threadId);

	if (thread == NULL)
	{
		logPrintf(LOG_LEVEL_INFO, "plugin", "CreateThread failed");
		return 1;
	}
	//sendEvent("clientAdd", "log from sissm", "");
	//WaitForSingleObject(thread, INFINITE);

#else

	pthread_t thread;

	if (pthread_create(&thread, NULL, thread_func, NULL) != 0)
	{
		logPrintf(LOG_LEVEL_INFO, "plugin", "CreateThread failed");
		return 1;
	}
	//sleep(10);
	//sendEvent("clientAdd", "log from sissm", "");

	//	if (pthread_join(thread, NULL) != 0)
	//	{
	//	   printf("pthread_join failed\n");
	//	    return 1;
	//	}

#endif
	return 0;

}



void getJsonObjectString(cJSON* root, char* name, char* resultData, int len) {
	cJSON* obj = cJSON_GetObjectItem(root, name);
	if (obj == NULL) {
		return "";
	}

	//return obj->valuestring;
	strlcpy(resultData, obj->valuestring, RESULT_DATA_SIZE);

}

int exeCmd(char* requestName, int paramsNum, char** paramsArray, char* resultData, char* resultMsg) {

	if (requestName == NULL) {
		strlcpy(resultMsg, "requestName is empty", RESULT_MSG_SIZE);
		return -1;
	}
	int targetParamsNum = 0;
	if (strcmp(requestName, "apiGameModePropertySet") == 0) {
		targetParamsNum = 2;
		if (paramsNum == targetParamsNum) {
			return apiGameModePropertySet(paramsArray[0], paramsArray[1]);
		}
	}
	if (strcmp(requestName, "apiGameModePropertyGet") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			char* result = apiGameModePropertyGet(paramsArray[0]);
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}
	if (strcmp(requestName, "apiSay") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			return apiSay(paramsArray[0]);
		}
	}
	if (strcmp(requestName, "apiSaySys") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			return apiSaySys(paramsArray[0]);
		}
	}
	if (strcmp(requestName, "apiKickOrBan") == 0) {
		targetParamsNum = 3;
		if (paramsNum == targetParamsNum) {
			int isBan = strtoi(paramsArray[0]);
			return apiKickOrBan(isBan, paramsArray[1], paramsArray[2]);
		}
	}
	if (strcmp(requestName, "apiKick") == 0) {
		targetParamsNum = 2;
		if (paramsNum == targetParamsNum) {
			return apiKick(paramsArray[0], paramsArray[1]);
		}
	}

	if (strcmp(requestName, "apiKickAll") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			return apiKickAll(paramsArray[0]);
		}
	}

	if (strcmp(requestName, "apiRcon") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			char rconResp[API_R_BUFSIZE];
			apiRcon(paramsArray[0], resultData);
		}
	}
	if (strcmp(requestName, "apiPlayersGetCount") == 0) {
		targetParamsNum = 0;
		if (paramsNum == targetParamsNum) {
			int playerCount = apiPlayersGetCount();
			snprintf(resultData, 10, "%d", playerCount);
		}
	}
	if (strcmp(requestName, "apiPlayersRoster") == 0) {
		targetParamsNum = 2;
		if (paramsNum == targetParamsNum) {
			int infoDepth = strtoi(paramsArray[0]);
			char* result = apiPlayersRoster(infoDepth, paramsArray[1]);
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}

	if (strcmp(requestName, "apiGetServerName") == 0) {
		targetParamsNum = 0;
		if (paramsNum == targetParamsNum) {
			char* result = apiGetServerName();
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}

	if (strcmp(requestName, "apiGetServerNameRCON") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			int forceCacheRefresh = strtoi(paramsArray[0]);
			char* result = apiGetServerNameRCON(forceCacheRefresh);
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}

	if (strcmp(requestName, "apiGetServerMode") == 0) {
		targetParamsNum = 0;
		if (paramsNum == targetParamsNum) {
			char* result = apiGetServerMode();
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}

	if (strcmp(requestName, "apiGetMapName") == 0) {
		targetParamsNum = 0;
		if (paramsNum == targetParamsNum) {
			char* result = apiGetMapName();
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}


	if (strcmp(requestName, "apiGetSessionID") == 0) {
		targetParamsNum = 0;
		if (paramsNum == targetParamsNum) {
			char* result = apiGetSessionID();
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}
	if (strcmp(requestName, "apiGetLastRosterTime") == 0) {
		targetParamsNum = 0;
		if (paramsNum == targetParamsNum) {
			int result = apiGetLastRosterTime();
			snprintf(resultData, 10, "%d", result);
		}
	}

	if (strcmp(requestName, "apiBadNameCheck") == 0) {
		targetParamsNum = 2;
		if (paramsNum == targetParamsNum) {
			int exactMatch = strtoi(paramsArray[1]);
			return  apiBadNameCheck(paramsArray[0], exactMatch);
		}
	}

	if (strcmp(requestName, "apiMapcycleRead") == 0) {
		targetParamsNum = 0;
		if (paramsNum == targetParamsNum) {
			return apiMapcycleRead(resultData);
		}
	}

	if (strcmp(requestName, "apiMapChange") == 0) {
		targetParamsNum = 5;
		if (paramsNum == targetParamsNum) {
			char* mapName = paramsArray[0];
			char* gameMode = paramsArray[1];
			int secIns = strtoi(paramsArray[2]);
			int dayNight = strtoi(paramsArray[3]);
			char* mutatorsList = paramsArray[4];
			return apiMapChange(mapName, gameMode, secIns, dayNight, mutatorsList, 0);
		}
	}

	if (strcmp(requestName, "apiMapList") == 0) {
		targetParamsNum = 0;
		if (paramsNum == targetParamsNum) {
			char* result = apiMapList();
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}
	if (strcmp(requestName, "apiMutList") == 0) {
		targetParamsNum = 0;
		if (paramsNum == targetParamsNum) {
			char* result = apiMutList();
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}

	if (strcmp(requestName, "apiIsSupportedGameMode") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			char* candidateMode = paramsArray[0];
			char* result = apiIsSupportedGameMode(candidateMode);
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}

	if (strcmp(requestName, "apiIsAdmin") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			char* connectID = paramsArray[0];
			int result = apiIsAdmin(connectID);
			snprintf(resultData, 10, "%d", result);
		}
	}

	if (strcmp(requestName, "apiNameToCharacter") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			char* playerName = paramsArray[0];
			char* result = apiNameToCharacter(playerName);
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}

	if (strcmp(requestName, "apiCharacterToName") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			char* characterID = paramsArray[0];
			char* result = apiCharacterToName(characterID);
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}

	if (strcmp(requestName, "apiIsPlayerAliveByName") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			char* playerName = paramsArray[0];
			int result = apiIsPlayerAliveByName(playerName);
			strlcpy(resultData, result, RESULT_DATA_SIZE);
		}
	}

	if (strcmp(requestName, "apiIsPlayerAliveByGUID") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			char* playerGUID = paramsArray[0];
			int result = apiIsPlayerAliveByGUID(playerGUID);
			snprintf(resultData, 10, "%d", result);
		}
	}

	if (strcmp(requestName, "apiLookupObjectiveLetterFromCache") == 0) {
		targetParamsNum = 1;
		if (paramsNum == targetParamsNum) {
			char* objectiveName = paramsArray[0];
			char result = apiLookupObjectiveLetterFromCache(objectiveName);
			char resultStr[1] = { result };
			strlcpy(resultData, resultStr, RESULT_DATA_SIZE);
		}
	}

	if (strcmp(requestName, "apiLookupLastObjectiveLetterFromCache") == 0) {
		targetParamsNum = 0;
		if (paramsNum == targetParamsNum) {
			char  result = apiLookupLastObjectiveLetterFromCache();
			char resultStr[1] = { result };
			strlcpy(resultData, resultStr, RESULT_DATA_SIZE);
		}
	}

	if (paramsNum != targetParamsNum) {
		snprintf(resultMsg, RESULT_MSG_SIZE, "requestParams num error:expect:%d,your:%d", targetParamsNum, paramsNum);
		return -1;
	}
	return 0;
}

void replyMsg(char* receiveMsg111) {
	char receiveMsg[2000];
	strlcpy(receiveMsg, receiveMsg111, 2000);
	if (strlen(receiveMsg) < 8) {
		return;
	}
	if (pluginConfig.enableLog == 1) {
		logPrintf(LOG_LEVEL_INFO, "plugin", "reveive json %s len=%d", receiveMsg, strlen(receiveMsg));
	}
	//解析json1
	//通过cJSON_Parse解析接收到的字符串，再通过cJSON_GetObjectItem获取指定键的值，最后释放该JSON结点的内存
	cJSON* root;
	root = cJSON_Parse(receiveMsg);
	char* requestType[50];
	strclr(requestType);
	getJsonObjectString(root, "requestType", requestType, 50);
	char* requestName[200];
	strclr(requestName);
	getJsonObjectString(root, "requestName", requestName, 200);
	char* requestParams[500];
	strclr(requestParams);
	getJsonObjectString(root, "requestParams", requestParams, 500);
	char* requestId[30];
	strclr(requestId);
	getJsonObjectString(root, "requestId", requestId, 30);

	cJSON* response;
	response = cJSON_CreateObject();
	cJSON_AddStringToObject(response, "requestType", "response");
	cJSON_AddStringToObject(response, "requestName", requestName);
	cJSON_AddStringToObject(response, "requestId", requestId);
	size_t paramsNum = 0;
	char* paramsArray[10] = { 0 };
	if (requestParams != NULL && strlen(requestParams) > 0) {
		split(requestParams, "|", paramsArray, &paramsNum);
		if (pluginConfig.enableLog == 1) {
			logPrintf(LOG_LEVEL_INFO, "plugin", "replyMsg requestParams=%s", requestParams);
			int i;
			for (i = 0; i < paramsNum; i++) {
				logPrintf(LOG_LEVEL_INFO, "plugin", "replyMsg paramsArray[%s]=", paramsArray[i]);
			}
		}
	}

	char resultData[RESULT_DATA_SIZE];
	char resultMsg[RESULT_MSG_SIZE];
	strclr(resultData);
	strclr(resultMsg);
	int errCode = exeCmd(requestName, paramsNum, paramsArray, resultData, resultMsg);
	cJSON_AddNumberToObject(response, "resultCode", errCode);


	cJSON_AddStringToObject(response, "resultMsg", resultMsg);
	cJSON_AddStringToObject(response, "resultData", resultData);

	char* szJSON = cJSON_PrintUnformatted(response);
	char responseJson[API_R_BUFSIZE + 500];
	strlcpy(responseJson, szJSON, API_R_BUFSIZE + 500);

	sendSocket(responseJson);

	free(szJSON);
	cJSON_Delete(root);
	cJSON_Delete(response);
}

void closeSocket1(int fd) {
	logPrintf(LOG_LEVEL_INFO, "plugin", "close socket");
#ifdef _WIN32
	closesocket(fd);
	closesocket(fd);
#else
	close(fd);
	close(fd);
#endif
}

int startSocket() {

	struct sockaddr_in address;
	int addrlen = sizeof(address);
	char buffer[MAX_BUFFER_SIZE] = { 0 };

	// 设置socket地址
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	aliveSockect = 1;
	char message[100], server_reply[2000];


	while (aliveSockect) {
		logPrintf(LOG_LEVEL_INFO, "plugin", "while loop start");
		closeSocket1(new_socket);

		if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			sleep(3);
			continue;
		}
		// allow reconnet at now, release port
		int opt = 1;
		int  err = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		if (err < 0) {
			sleep(3);
			continue;
		}

		if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
			logPrintf(LOG_LEVEL_INFO, "plugin", "bind failed");
			sleep(3);
			continue;
		}
		if (listen(server_fd, 3) < 0) {
			logPrintf(LOG_LEVEL_INFO, "plugin", "listen failed");
			continue;
		}

		logPrintf(LOG_LEVEL_INFO, "plugin", "Waiting for incoming connections...");
		// 接受客户端连接
		if ((new_socket = accept(server_fd, &address, &addrlen)) < 0) {
			logPrintf(LOG_LEVEL_INFO, "plugin", "accept failed");
			sleep(3);
			continue;
		}

		logPrintf(LOG_LEVEL_INFO, "plugin", "Client connected");
		if (new_socket == INVALID_SOCKET)
		{
			logPrintf(LOG_LEVEL_INFO, "plugin", "accept socket error");
			sleep(5);
			continue;
		}
		else
		{
			socketConnected = 1;
			needReConnect = 0;
			logPrintf(LOG_LEVEL_INFO, "plugin", "Connected");
			while (aliveSockect)
			{
				strclr(server_reply);
				int len = recv(new_socket, server_reply, 2000, 0);
				if (len < 0)
				{
					logPrintf(LOG_LEVEL_INFO, "plugin", "recv failed");
					socketConnected = 0;
					break;
				}
				server_reply[len] = '\0';

				replyMsg(server_reply);

				if (needReConnect == 1) {
					socketConnected = 0;
					break;
				}
			}

		}

	}
	printf("exit socket thread\n");
	closeSocket1(new_socket);
	closeSocket1(server_fd);
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}


//  ==============================================================================================
//  pluginClientAddCB
//
//  Add client event handler 
//  This only works reliably when EasyAC is on.  For slower but reliable event use
//  the Syntehtic-Add Callback.
//
int pluginClientAddCB(char* strIn)
{
	char playerName[256], playerGUID[256], playerIP[256];

	rosterParsePlayerConn(strIn, 256, playerName, playerGUID, playerIP);
	char otherJson[200];
	snprintf(otherJson, 200, ", \"playerName\":\"%s\",\"playerGUID\":\"%s\",\"playerIP\":\"%s\"", playerName, playerGUID, playerIP);

	sendEvent("clientAdd", strIn, otherJson);
	return 0;
}

//  ==============================================================================================
//  pluginClientDelCB
//
//  Del client event handler 
//  This only works reliably when EasyAC is on.  For slower but reliable event use
//  the Syntehtic-Del Callback.
//
int pluginClientDelCB(char* strIn)
{
	char playerName[256], playerGUID[256], playerIP[256];

	rosterParsePlayerDisConn(strIn, 256, playerName, playerGUID, playerIP);
	//apiSay ( "plugin: Good bye %s", playerName );
	sendEvent("clientDel", strIn, NULL);
	return 0;
}

//  ==============================================================================================
//  pluginInitCB
//
//  This callback is invoked whenever SISSM is started or reset.
//
int pluginInitCB(char* strIn)
{
	sendEvent("init", strIn, NULL);
	return 0;
}

//  ==============================================================================================
//  pluginRestartCB
//
//  This callback is invoked just before restart/reboot is issued to the game server.
//  Expect comms blackout for 10-30 seconds after this while the game server reboots.
//
int pluginRestartCB(char* strIn)
{
	sendEvent("restart", strIn, NULL);
	return 0;
}

//  ==============================================================================================
//  pluginMapChangeCB
//
//  This callback is invoked whenever a map change is detected.
//
int pluginMapChangeCB(char* strIn)
{
	sendEvent("mapChange", strIn, NULL);
	return 0;
}

//  ==============================================================================================
//  pluginGameStartCB
//
//  This callback is invoked whenever a Game-Start event is detected.
//
int pluginGameStartCB(char* strIn)
{
	// in-game announcement start of game
	//apiSay( "plugin: %s -- 2 waves of %s bots", pluginConfig.stringParameterExample, newCount );  
	sendEvent("gameStart", strIn, NULL);
	return 0;
}

//  ==============================================================================================
//  pluginGameEndCB
//
//  This callback is invoked whenever a Game-End event is detected.
//
int pluginGameEndCB(char* strIn)
{
	sendEvent("gameEnd", strIn, NULL);
	return 0;
}

//  ==============================================================================================
//  pluginRoundStartCB
//
//  This callback is invoked whenver a Round-Start event is detected.
//
int pluginRoundStartCB(char* strIn)
{
	sendEvent("roundStart", strIn, NULL);
	return 0;
}

//  ==============================================================================================
//  pluginRoundEndCB
//
//  This callback is invoked whenever a End-of-round event is detected.
//
int pluginRoundEndCB(char* strIn)
{
	sendEvent("roundEnd", strIn, NULL);
	return 0;
}

//  ==============================================================================================
//  pluginCaptureCB
//
//  This callback is invoked whenever an objective is captured.  Note multiple call of this
//  may occur for each objective.  To generate a single event, following example filters
//  all but the first event within N seconds.
//
int pluginCapturedCB(char* strIn)
{
	static unsigned long int lastTimeCaptured = 0L;
	// system generates multiple 'captured' report, so it is necessary
	// to add a 10-second window filter to make sure only one gets fired off.
	//
	if (lastTimeCaptured + 10L < apiTimeGet()) {

		lastTimeCaptured = apiTimeGet();
		// apiServerRestart();
		sendEvent("captured", strIn, NULL);
	}


	return 0;
}

//  ==============================================================================================
//  pluginPeriodicCB
//
//  This callback is invoked at 1.0Hz rate (once a second).  Due to serial processing,
//  exact timing cannot be guaranteed.
//
int pluginPeriodicCB(char* strIn)
{
	return 0;
}


//  ==============================================================================================
//  pluginShutdownCB
//
//  This callback is invoked whenever a game shutdown is detected, typically as a result of 
//  restart request, or operator terminating the server.   This event is not a SISSM shutdown.
//
int pluginShutdownCB(char* strIn)
{
	sendEvent("shutdown", strIn, NULL);
	return 0;
}

//  ==============================================================================================
//  pluginClientSynthDelCB
//
//  This is a slower but more reliable version of Player deletion event.  Unlike ClientDEL event
//  this CB is generated by comparing the RCON roster listplayer dumps.  This CB does
//  not require easyAC server to be operational.
//
//  strIn:  in the format: "~SYNTHDEL~ 76561000000000000 001.002.003.004 NameOfPlayer"
//
int pluginClientSynthDelCB(char* strIn)
{
	static char playerName[256], playerGUID[256], playerIP[256];

	rosterParsePlayerSynthDisConn(strIn, 256, playerName, playerGUID, playerIP);
	char otherJson[200];
	snprintf(otherJson, 200, ", \"playerName\":\"%s\",\"playerGUID\":\"%s\",\"playerIP\":\"%s\"", playerName, playerGUID, playerIP);

	sendEvent("clientSynthDel", strIn, otherJson);
	return 0;
}

//  ==============================================================================================
//  pluginClientSynthAddCB
//
//  This is a slower but more reliable version of Player add event.  Unlike ClientADD event
//  this CB is generated by comparing the RCON roster listplayer dumps.  This CB does
//  not require easyAC server to be operational.
//
//  strIn:  in the format: "~SYNTHADD~ 76561000000000000 001.002.003.004 NameOfPlayer"
//
int pluginClientSynthAddCB(char* strIn)
{
	static char playerName[256], playerGUID[256], playerIP[256];

	rosterParsePlayerSynthConn(strIn, 256, playerName, playerGUID, playerIP);

	char otherJson[200];
	snprintf(otherJson, 200, ", \"playerName\":\"%s\",\"playerGUID\":\"%s\",\"playerIP\":\"%s\"", playerName, playerGUID, playerIP);

	sendEvent("clientSynthAdd", strIn, otherJson);

	return 0;
}

//  ==============================================================================================
//  pluginChatCB           
//
//  This callback is invoked whenever any player enters a text chat message.                  
//
int pluginChatCB(char* strIn)
{
	sendEvent("chat", strIn, NULL);
	return 0;
}

//  ==============================================================================================
//  pluginSigtermCB           
//
//  This callback is invoked when SISSM is terminated abnormally (either by OS or operator
//  pressing ^C.  Since the game server is assumed running, plugins should take necessary action
//  to leave the server in 'playable state' in absense of SISSM.
//
int pluginSigtermCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "plugin", "SISSM Termination CB");
	sendEvent("sigterm", strIn, NULL);
	return 0;
}

//  ==============================================================================================
//  plugin
//
//
int pluginWinLose(char* strIn)
{
	char otherJson[200];
	snprintf(otherJson, 200, ", \"humanSide\":\"%d\"", rosterGetCoopSide());

	sendEvent("winLose", strIn, otherJson);
	return 0;
}

//  ==============================================================================================
//  plugin
//
//
int pluginTravel(char* strIn)
{
	sendEvent("travel", strIn, NULL);

	return 0;
}

//  ==============================================================================================
//  plugin
//
//
int pluginSessionLog(char* strIn)
{
	char* sessionID;

	sessionID = rosterGetSessionID();

	char otherJson[100];
	snprintf(otherJson, 100, ", \"sessionID\":\"%s\"", sessionID);

	sendEvent("session", strIn, otherJson);
	return 0;
}

//  ==============================================================================================
//  plugin
//
//
int pluginObjectSynth(char* strIn)
{
	char* obj, * typ;

	obj = rosterGetObjective();
	typ = rosterGetObjectiveType();

	char otherJson[100];
	snprintf(otherJson, 100, ", \"obj\":\"%s\", \"typ\":\"%s\"", obj, typ);

	sendEvent("objectSynth", strIn, otherJson);
	return 0;
}

int pluginKilled(char* strIn)
{// TODO 后面解析一下
	sendEvent("killed", strIn, "");
	return 0;
}
int pluginTakeObject(char* strIn)
{// TODO 后面解析一下
	sendEvent("takeObject", strIn, "");
	return 0;
}

int pluginRoundStateChange(char* strIn)
{// TODO 后面解析一下
	sendEvent("roundStateChange", strIn, "");
	return 0;
}
int pluginEveryLog(char* strIn)
{


	if (matchEventMaxIndex == -1) {
		return;
	}
	int i;
	for (i = 0; i <= matchEventMaxIndex; i++) {
		if (strlen(pluginConfig.matchEventString[i]) > 0) {

			if (NULL != strstr(strIn, pluginConfig.matchEventString[i])) {
				sendEvent("everyLog", strIn, "");
				break;
			}
		}
	}

	return 0;
}
//  ==============================================================================================
//  ...
//
//  ...
//  This method is exported and is called from the main sissm module.
//
int pluginInstallPlugin(void)
{
	logPrintf(LOG_LEVEL_INFO, "plugin", "pluginInstallPlugin");
	// Read the plugin-specific variables from the .cfg file 
	// 
	pluginInitConfig();

	// if plugin is disabled in the .cfg file then do not activate
	//
	if (pluginConfig.pluginState == 0) return 0;

	// Install Event-driven CallBack hooks so the plugin gets
	// notified for various happenings.  A complete list is here,
	// but comment out what is not needed for your plug-in.
	// 
	eventsRegister(SISSM_EV_CLIENT_ADD, pluginClientAddCB);
	eventsRegister(SISSM_EV_CLIENT_DEL, pluginClientDelCB);
	eventsRegister(SISSM_EV_INIT, pluginInitCB);
	eventsRegister(SISSM_EV_RESTART, pluginRestartCB);
	eventsRegister(SISSM_EV_MAPCHANGE, pluginMapChangeCB);
	eventsRegister(SISSM_EV_GAME_START, pluginGameStartCB);
	//eventsRegister( SISSM_EV_GAME_END,             pluginGameEndCB );// not working
	eventsRegister(SISSM_EV_GAME_END_NOW, pluginGameEndCB);
	eventsRegister(SISSM_EV_ROUND_START, pluginRoundStartCB);
	eventsRegister(SISSM_EV_ROUND_END, pluginRoundEndCB);
	eventsRegister(SISSM_EV_OBJECTIVE_CAPTURED, pluginCapturedCB);
	eventsRegister(SISSM_EV_PERIODIC, pluginPeriodicCB);
	eventsRegister(SISSM_EV_SHUTDOWN, pluginShutdownCB);
	eventsRegister(SISSM_EV_CLIENT_ADD_SYNTH, pluginClientSynthAddCB);
	eventsRegister(SISSM_EV_CLIENT_DEL_SYNTH, pluginClientSynthDelCB);
	eventsRegister(SISSM_EV_CHAT, pluginChatCB);
	eventsRegister(SISSM_EV_SIGTERM, pluginSigtermCB);
	eventsRegister(SISSM_EV_WINLOSE, pluginWinLose);
	eventsRegister(SISSM_EV_TRAVEL, pluginTravel);
	eventsRegister(SISSM_EV_SESSIONLOG, pluginSessionLog);
	eventsRegister(SISSM_EV_OBJECT_SYNTH, pluginObjectSynth);
	eventsRegister(SISSM_EV_KILLED, pluginKilled);
	eventsRegister(SISSM_EV_SS_TAKE_OBJECTIVE, pluginTakeObject);
	eventsRegister(SISSM_EV_SS_ROUND_STATE_CHANGE, pluginRoundStateChange);
	eventsRegister(SISSM_EV_EVERY_EVENT, pluginEveryLog);

	// start sockect thread
	startThread();

	return 0;
}


