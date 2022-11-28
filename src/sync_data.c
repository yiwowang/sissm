//  ==============================================================================================
//
//  Module: map_vote
//
//  Description:
//  players vote to change map
//
//  Original Author:
//  Yeye    2022.09.12
//
//  Released under MIT License
//  ID Authenticator: c4c5a1eda6815f65bb2eefd15c5b5058f996add99fa8800831599a7eb5c2a04c
//
//  ==============================================================================================

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
#include "time.h"

#include "roster.h"
#include "api.h"
#include "sissm.h"

#include "sync_data.h"
#include <ctype.h>
#include "http.h"
#include "cJSON.h"
#include "common_util.h"
//  ==============================================================================================
//  Data definition
//
static struct
{
	int pluginState; // always have this in .cfg file:  0=disabled 1=enabled
	char httpUrl[200];
	char outputPath[200];
	int serverId;
} syncDataConfig;

#define MAP_COUNT 16
#define ROOM_PLAYER_MAX_COUNT 20

struct KillWay {
	char name[50];
	int count;
};

struct PlayerData
{
	char name[30];// 玩家名字
	char uid[20];// 玩家steamId
	int score;// 玩家得分
	char ip[15];// ip
	int killCount;// 杀敌数量
	int deadCount;// 死亡数量
	int assistCount;// 助攻数量
	int suicideCount;// 自杀数量
	int takeCount;// 站点数量
	long joinTime;// 加入时间
	long leaveTime;// 离开时间

	struct KillWay killWay[300];
	int killWayIndex;

	struct KillWay deadWay[20];
	int deadWayIndex;
};

struct PlayerData  playerList[50];
int playerDataIndex = -1;
long gameStartTime;
int gameRountLimit;
int gameIsFull;
char gameMapName[256];
struct RoundData
{
	int serverId;// 几服
	long gameStartTime;//此轮开始时间
	int gameRountLimit;
	int gameIsFull;// 是否完整的游戏开始和结束
	int roundId;// 第几局
	long roundStartTime;
	long roundEndTime;
	char mapName[50];
	int camp;// 阵营 1=政府军
	int win;
	char  endReason[50];
	char takePosition[10];// 占领到哪个点了
};
struct RoundData roundData;



struct ChatData {
	char uid[20];
	char name[30];// 不用存
	char msg[100];
	long time;
};

struct ChatData  chatList[500];
int chatDataIndex = -1;

void roundInit() {
	for (int i = 0; i <= playerDataIndex; i++) {
		playerList[i].score = 0;
		playerList[i].killCount = 0;
		playerList[i].deadCount = 0;
		playerList[i].assistCount = 0;
		playerList[i].suicideCount = 0;
		playerList[i].killWayIndex = -1;
		playerList[i].deadWayIndex = -1;
		playerList[i].joinTime = 0;
		playerList[i].leaveTime = 0;
		playerList[i].takeCount = 0;
	}
	playerDataIndex = -1;

}

struct PlayerData* playerGet(char* uid) {
	for (int i = 0; i <= playerDataIndex; i++) {
		if (strcmp(playerList[i].uid, uid) == 0) {
			return &playerList[i];
		}
	}
	return NULL;
}

struct PlayerData* playerGetOrCreate(char* uid, char* name) {
	if (!isValidUid(uid)) {
		return NULL;
	}
	struct PlayerData* item = playerGet(uid);
	if (item == NULL) {
		item = &playerList[++playerDataIndex];
		strlcpy(item->uid, uid, sizeof(item->uid));
		strlcpy(item->name, name, sizeof(item->name));
		item->joinTime = apiTimeGet();
	}
	return item;
}

void syncPlayerData() {
	for (int i = 0; i < ROSTER_MAX; i++) {
		rconRoster_t* p = getMasterRoster(i);
		if (p == NULL || strlen(p->steamID) <= 0) {
			break;
		}

		struct PlayerData* player = playerGetOrCreate(p->steamID, p->playerName);
		if (player == NULL) {
			break;
		}
		strlcpy(player->name, p->playerName, sizeof(player->name));
		strlcpy(player->ip, p->IPaddress, sizeof(player->ip));
		player->score = strtoi(p->score);
	}
}


int createRoundJson(char* strJson, int size, int format) {
	cJSON* pRoot = cJSON_CreateObject();

	cJSON* pRound = cJSON_CreateObject();

	cJSON_AddNumberToObject(pRound, "serverId", roundData.serverId);
	cJSON_AddNumberToObject(pRound, "gameIsFull", roundData.gameIsFull);
	cJSON_AddNumberToObject(pRound, "gameStartTime", roundData.gameStartTime);
	cJSON_AddNumberToObject(pRound, "gameRountLimit", roundData.gameRountLimit);

	cJSON_AddNumberToObject(pRound, "roundId", roundData.roundId);
	cJSON_AddNumberToObject(pRound, "roundStartTime", roundData.roundStartTime);
	cJSON_AddNumberToObject(pRound, "roundEndTime", roundData.roundEndTime);
	cJSON_AddStringToObject(pRound, "mapName", roundData.mapName);
	cJSON_AddNumberToObject(pRound, "camp", roundData.camp);
	cJSON_AddNumberToObject(pRound, "win", roundData.win);
	cJSON_AddStringToObject(pRound, "endReason", roundData.endReason);
	cJSON_AddStringToObject(pRound, "takePosition", roundData.takePosition);
	cJSON_AddItemToObject(pRoot, "roundData", pRound);
	// playerList
	cJSON* pPlayerArray = cJSON_CreateArray();
	for (int i = 0; i <= playerDataIndex; i++) {
		cJSON* pItem = cJSON_CreateObject();
		cJSON_AddStringToObject(pItem, "name", playerList[i].name);
		cJSON_AddStringToObject(pItem, "uid", playerList[i].uid);
		cJSON_AddNumberToObject(pItem, "score", playerList[i].score);
		cJSON_AddNumberToObject(pItem, "killCount", playerList[i].killCount);
		cJSON_AddNumberToObject(pItem, "deadCount", playerList[i].deadCount);
		cJSON_AddNumberToObject(pItem, "assistCount", playerList[i].assistCount);
		cJSON_AddNumberToObject(pItem, "suicideCount", playerList[i].suicideCount);
		cJSON_AddNumberToObject(pItem, "takeCount", playerList[i].takeCount);
		cJSON_AddNumberToObject(pItem, "joinTime", playerList[i].joinTime);
		cJSON_AddNumberToObject(pItem, "leaveTime", playerList[i].leaveTime);
		cJSON_AddStringToObject(pItem, "ip", playerList[i].ip);

		cJSON* pKillWay = cJSON_CreateObject();
		for (int j = 0; j <= playerList[i].killWayIndex; j++) {
			cJSON_AddNumberToObject(pKillWay, playerList[i].killWay[j].name, playerList[i].killWay[j].count);
		}
		cJSON_AddItemToObject(pItem, "killWay", pKillWay);

		cJSON* pDeadWay = cJSON_CreateObject();
		for (int j = 0; j <= playerList[i].deadWayIndex; j++) {
			cJSON_AddNumberToObject(pDeadWay, playerList[i].deadWay[j].name, playerList[i].deadWay[j].count);
		}
		cJSON_AddItemToObject(pItem, "deadWay", pDeadWay);


		cJSON_AddItemToArray(pPlayerArray, pItem);
	}

	cJSON_AddItemToObject(pRoot, "playerList", pPlayerArray);




	cJSON* pChatArray = cJSON_CreateArray();
	for (int i = 0; i <= chatDataIndex; i++) {
		cJSON* pItem = cJSON_CreateObject();
		cJSON_AddStringToObject(pItem, "uid", chatList[i].uid);
		// 后续不用传name
		cJSON_AddStringToObject(pItem, "name", chatList[i].name);
		cJSON_AddStringToObject(pItem, "msg", chatList[i].msg);
		cJSON_AddNumberToObject(pItem, "time", chatList[i].time);
		cJSON_AddItemToArray(pChatArray, pItem);
	}

	cJSON_AddItemToObject(pRoot, "chatList", pChatArray);

	if (format) {
		char* szJSON = cJSON_Print(pRoot);
		strlcpy(strJson, szJSON, size);
		free(szJSON);
		cJSON_Delete(pRoot);
	}
	else {
		char* szJSON = cJSON_PrintUnformatted(pRoot);
		strlcpy(strJson, szJSON, size);
		free(szJSON);
		cJSON_Delete(pRoot);
	}

	return 0;
}

//  ==============================================================================================
//  InitConfig
//
//  Read parameters from the .cfg file
//
int syncDataInitConfig(void)
{
	cfsPtr cP;

	cP = cfsCreate(sissmGetConfigPath());
	// read "map_vote.pluginstate" variable from the .cfg file
	syncDataConfig.pluginState = (int)cfsFetchNum(cP, "sync_data.pluginState", 0.0); // disabled by default
	syncDataConfig.serverId = (int)cfsFetchNum(cP, "sync_data.serverId", 0);
	strlcpy(syncDataConfig.httpUrl, cfsFetchStr(cP, "sync_data.httpUrl", ""), CFS_FETCH_MAX);
	strlcpy(syncDataConfig.outputPath, cfsFetchStr(cP, "sync_data.outputPath", ""), CFS_FETCH_MAX);

	cfsDestroy(cP);
	return 0;
}

//  ==============================================================================================
// InitCB
//
//  This callback is invoked whenever SISSM is started or reset.
//
int syncDataInitCB(char* strIn)
{

	roundInit();
	logPrintf(LOG_LEVEL_INFO, "stncData", "Init Event ::%s::", strIn);
	return 0;
}


int syncDataClientSynthAddCB(char* strIn)
{
	syncPlayerData();

	static char playerName[256], playerGUID[256], playerIP[256];

	rosterParsePlayerSynthConn(strIn, 256, playerName, playerGUID, playerIP);
	logPrintf(LOG_LEVEL_INFO, "syncData", "Synthetic ADD Callback Name ::%s:: IP ::%s:: GUID ::%s::", playerName, playerIP, playerGUID);
	return 0;
}

int syncDataClientSynthDelCB(char* strIn)
{

	char playerName[256], playerGUID[256], playerIP[256];

	rosterParsePlayerDisConn(strIn, 256, playerName, playerGUID, playerIP);
	logPrintf(LOG_LEVEL_INFO, "syncData", "Synthetic DEL Callback Name ::%s:: IP ::%s:: GUID ::%s::", playerName, playerIP, playerGUID);
	struct PlayerData* player = playerGetOrCreate(playerGUID, playerName);
	if (player != NULL) {

		player->leaveTime = apiTimeGet();
	}
	return 0;
}


void initGameStart() {
	gameStartTime = apiTimeGet();

	char* strRoundLimit = apiGameModePropertyGet("RoundLimit");
	if (strlen(strRoundLimit) > 0) {
		printf("strRoundLimit=====%s====\n", strRoundLimit);
		gameRountLimit = strtoi(strRoundLimit);
	}

	char* mapName = apiGetMapName();
	if (strlen(mapName) > 0) {
		printf("mapName=====%s====\n", mapName);
		if (strstr(mapName, "/Game/Maps/") != NULL) {
			getWordRight(mapName, "/Game/Maps/", roundData.mapName);
		}
		else if (strstr(mapName, "_Checkpoint") != NULL) {
			getWordLeft(mapName, "_Checkpoint", roundData.mapName);
		}
		else {
			strlcpy(roundData.mapName, mapName, sizeof(roundData.mapName));
		}
	}
}

int syncDataGameStartCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "syncData", "Game Start Event ::%s::", strIn);

	initGameStart();
	gameIsFull = 1;
	return 0;
}


//  ==============================================================================================
//  GameEndCB
//
//  This callback is invoked whenever a Game-End event is detected.
//
int syncDataGameEndCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "map_vote", "Game End Event ::%s::", strIn);
	gameStartTime = 0;
	gameIsFull = 0;
	return 0;
}

//  ==============================================================================================
//  syncDataRoundStart
//
//
int syncDataRoundStart(int round)
{
	roundInit();

	syncPlayerData();

	if (gameStartTime <= 0) {
		gameIsFull = 0;
		initGameStart();
	}

	roundData.gameStartTime = gameStartTime;
	roundData.gameIsFull = gameIsFull;
	roundData.serverId = syncDataConfig.serverId;
	roundData.gameRountLimit = gameRountLimit;
	roundData.roundId = round;
	roundData.roundStartTime = apiTimeGet();
	return 0;
}

//  ==============================================================================================
//  syncDataRoundEnd
//
//
int syncDataRoundEnd(int round, int win, char* endReason)
{
	roundData.roundEndTime = apiTimeGet();
	roundData.win = win;
	roundData.camp = rosterGetCoopSide() == 0 ? 1 : 2;
	strlcpy(roundData.endReason, endReason, sizeof(roundData.endReason));

	syncPlayerData();

	char  json[20 * 1024];
	createRoundJson(json, sizeof(json), 0);
	logPrintf(LOG_LEVEL_INFO, "syncData", "syncDataRoundEndCB  json:\n%s", json);

	if (exists(syncDataConfig.outputPath)) {
		char timeStr[50];
		getTimeStr(gameStartTime, "%d-%d-%d-%d-%d-%d", timeStr, sizeof(timeStr));

		char fileName[100];
		snprintf(fileName, 100, "%s%s-round-%d.txt", syncDataConfig.outputPath, timeStr, round);
		writeFile(fileName, json);
	}
	else {
		logPrintf(LOG_LEVEL_INFO, "syncData", "syncDataRoundEndCB outputPath not exists:%s", syncDataConfig.outputPath);
	}

	if (strlen(syncDataConfig.httpUrl) > 0) {
		logPrintf(LOG_LEVEL_INFO, "syncData", "httpRequest sending...");
		char responseBody[10 * 1024];
		httpRequest(5000, 2, "82.156.36.121", "/api.php", json, responseBody);
		logPrintf(LOG_LEVEL_INFO, "syncData", "httpRequest responseBody=%s\n", responseBody);
	}
	else {
		logPrintf(LOG_LEVEL_INFO, "syncData", "syncDataRoundEndCB syncDataConfig.httpUrl empty:%s", syncDataConfig.httpUrl);
	}



	roundData.roundId = -1;
	roundData.roundStartTime = -1;
	roundData.roundEndTime = -1;
	roundData.win = -1;
	strclr(roundData.takePosition);
	strclr(roundData.endReason);

	for (int i = 0; i < chatDataIndex; i++) {
		strclr(chatList[i].uid);
		strclr(chatList[i].name);
		strclr(chatList[i].msg);
		chatList[i].time = 0;
	}
	chatDataIndex = -1;
	// 发送
	return 0;
}


int syncDataRoundStateChangeCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "syncData", "syncDataRoundStateChangeCB :: % s::", strIn);
	if (strstr(strIn, "started") != NULL) {
		// LogGameplayEvents: Display: Round 4 started
		char round[10];
		getWordRange(strIn, " Round ", " started", round);

		syncDataRoundStart(strtoi(round));
	}
	else {
		// LogGameplayEvents: Display: Round 4 Over: Team 0 won (win reason: Elimination)
		char round[10];

		getWordRange(strIn, " Round ", " Over", round);

		char endReason[50];
		getWordRange(strIn, " reason: ", ")", endReason);

		int humanSide, win;

		humanSide = rosterGetCoopSide();
		if (NULL != strstr(strIn, "Team 0")) {
			win = humanSide == 0 ? 1 : 2;
		}
		else if (NULL != strstr(strIn, "Team 1")) {
			win = humanSide == 1 ? 1 : 2;
		}
		else {
			win = 3;
		}
		syncDataRoundEnd(strtoi(round), win, endReason);
	}

	return 0;
}



/*
* 统计击杀武器和次数，以及被杀武器和数量
*/
void countKillWay(struct KillWay  killWayArray[], int* killWayIndex, char* weaponName) {
	if (strlen(weaponName) <= 0) {
		return;
	}
	char finalWeaponName[50];
	if (strstr(weaponName, "Checkpoint") != NULL) {
		strlcpy(finalWeaponName, "Checkpoint", sizeof(finalWeaponName));
	}
	else {
		strlcpy(finalWeaponName, weaponName, sizeof(finalWeaponName));
	}
	for (int i = 0; i <= (*killWayIndex); i++) {
		if (strcmp(killWayArray[i].name, finalWeaponName) == 0) {
			killWayArray[i].count++;
			return;
		}
	}
	(*killWayIndex)++;

	killWayArray[(*killWayIndex)].count = 1;
	strlcpy(killWayArray[(*killWayIndex)].name, finalWeaponName, sizeof(killWayArray[(*killWayIndex)].name));
}

int syncDataKilledCB(char* strIn)
{
	//logPrintf(LOG_LEVEL_INFO, "kill_self", "killSelfKilled Event ::%s::", strIn);


	struct KillInfo* killInfo = NULL;
	killInfo = (struct KillInfo*)malloc(sizeof(*killInfo));
	parseKillInfo(strIn, killInfo);

	// 杀敌统计
	for (int i = 0; i <= killInfo->playerIndex; i++) {
		char* playerUid = killInfo->playerUid[i];
		char* playerName = killInfo->playerName[i];
		if (!isValidUid(playerUid)) {
			continue;
		}

		struct PlayerData* player = playerGetOrCreate(playerUid, playerName);
		int suicide = strcmp(playerUid, killInfo->deadUid) == 0 ? 1 : 0;
		if (player != NULL) {
			if (suicide == 1) {
				player->suicideCount++;
			}
			else if (i == 0) {
				player->killCount++;
				countKillWay(&player->killWay, &player->killWayIndex, killInfo->weaponName);
			}
			else {
				player->assistCount++;
			}
		}
	}

	// 死亡统计
	struct PlayerData* player = playerGetOrCreate(killInfo->deadUid, killInfo->deadName);
	if (player != NULL) {
		player->deadCount++;
		countKillWay(&player->deadWay, &player->deadWayIndex, killInfo->weaponName);
	}

	return 0;
}

int syncDataTakeObjectiveCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "sync_data", "syncDataTakeObjectiveCB Event ::%s::", strIn);
	char point[10];
	char rightStr[300];
	strclr(point);
	strclr(rightStr);

	if (strstr(strIn, " was destroyed for team ") != NULL) {
		getWordRange(strIn, "Display: Objective ", " owned by team ", point);
		getWordRight(strIn, " was destroyed for team ", rightStr);
	}

	if (strstr(strIn, " was captured for team ") != NULL) {
		getWordRange(strIn, "Display: Objective ", " was captured for team ", point);
		getWordRight(strIn, " was captured for team ", rightStr);
	}
	if (strlen(point) <= 0 || strlen(rightStr) <= 0) {
		return 0;
	}
	strlcpy(roundData.takePosition, point, sizeof(roundData.takePosition));
	logPrintf(LOG_LEVEL_INFO, "sync_data", "syncDataTakeObjectiveCB========%d===%s==%s===", strlen(point), point, roundData.takePosition);


	char playerStr[200];
	getWordRight(rightStr, " by ", playerStr);


	char* playerArray[20] = { 0 };
	size_t num = 0;


	split(playerStr, ", ", playerArray, &num);
	for (int i = 0; i < num; i++) {
		char uid[20];
		char name[50];
		getWordRange(playerArray[i], "[", "]", uid);
		getWordLeft(playerArray[i], "[", name);
		struct PlayerData* player = playerGetOrCreate(uid, name);
		if (player != NULL) {
			player->takeCount++;
		}
	}
	return 0;
}


void readLogFile(char* path)
{
	FILE* fp = NULL;
	//读写方式打开，如果文件不存在，打开失败
	fp = fopen(path, "r+");
	if (fp == NULL)
	{
		perror("my_fgets fopen");
		return;
	}

	char buf[1000];//char buf[100] = { 0 };
	while (!feof(fp))//文件没有结束
	{
		//sizeof(buf),最大值，放不下只能放100；如果不超过100，按实际大小存放
		//返回值，成功读取文件内容
		//会把“\n”读取，以“\n”作为换行的标志
		//fgets()读取完毕后，自动加字符串结束符0
		char* p = fgets(buf, sizeof(buf), fp);
		if (p != NULL)
		{
			strcat(buf, "\0");
			eventsDispatch(buf);
			/*printf("buf = %s\n", buf);
			printf("%s\n", p);*/
		}

	}
	printf("\n");

	if (fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}
}


int syncDataChatCB(char* strIn)
{
	// [2022.11.27 - 09.17.47:750] [695] LogChat: Display: 真主(76561198257228155) Global Chat : 2f
	// [2022.11.22-15.50.58:109][717]LogChat: Display: 12324YEYE(76561198097876746) Team 0 Chat: --help
	// [2022.11.22-17.11.04:888][250]LogChat: Display: AAAAA(76561198320645347) Team 1 Chat: 6
	//logPrintf(LOG_LEVEL_INFO, "syncData", "Client chat ::%s::", strIn);
	if (strstr(strIn,"LogChat: Display: ")==NULL) {
		logPrintf(LOG_LEVEL_INFO, "sync_data", "syncDataChatCB Error1 %s", strIn);
		return 0;
	}

	char chatStr[60];
	getWordRight(strIn,"LogChat: Display: ", chatStr);

	if (strstr(chatStr, "(") == NULL|| strstr(chatStr, ")") == NULL || strstr(chatStr, ": ") == NULL) {
		logPrintf(LOG_LEVEL_INFO, "sync_data", "syncDataChatCB Error1 %s", strIn);
		return 0;
	}

	char playerName[30];
	getWordLeft(chatStr, "(", playerName);
	char playerUid[20];
	getWordRange(chatStr, "(",")", playerUid);
	char msg[100];
	getWordRight(chatStr, ": ", msg);
	//printf("playerName=%s=   playerUid=%s=msg=%s=\n", playerName, playerUid, msg);
	struct ChatData* item = &chatList[++chatDataIndex];
	strlcpy(item->uid, playerUid, sizeof(item->uid));
	strlcpy(item->name, playerName, sizeof(item->name));
	strlcpy(item->msg, msg, sizeof(item->msg));
	item->time = apiTimeGet();

	return 0;
}



//  ==============================================================================================
//  ...
//
//  ...
//  This method is exported and is called from the main sissm module.
//
int syncDataInstallPlugin(void)
{
	logPrintf(LOG_LEVEL_INFO, "sync_data", "syncDataInstallPlugin");

	syncDataInitConfig();
	if (syncDataConfig.pluginState == 0) {
		return 0;
	}
	
	/*eventsRegister(SISSM_EV_CLIENT_ADD, syncDataClientAddCB);
eventsRegister(SISSM_EV_CLIENT_DE, syncDataClientDelCB);*/
	eventsRegister(SISSM_EV_INIT, syncDataInitCB);
	//eventsRegister(SISSM_EV_RESTART, syncDataRestartCB);
	//eventsRegister(SISSM_EV_MAPCHANGE, syncDataMapChangeCB);
	eventsRegister(SISSM_EV_GAME_START, syncDataGameStartCB);
	eventsRegister(SISSM_EV_GAME_END_NOW, syncDataGameEndCB);

	eventsRegister(SISSM_EV_SS_ROUND_STATE_CHANGE, syncDataRoundStateChangeCB);

	//eventsRegister(SISSM_EV_ROUND_START, syncDataRoundStartCB);
	//eventsRegister(SISSM_EV_ROUND_END, syncDataRoundEndCB);
	//eventsRegister(SISSM_EV_OBJECTIVE_CAPTURED, syncDataCapturedCB);
	//eventsRegister(SISSM_EV_PERIODIC, syncDataPeriodicCB);
	//eventsRegister(SISSM_EV_SHUTDOWN, syncDataShutdownCB);
	eventsRegister(SISSM_EV_CLIENT_ADD_SYNTH, syncDataClientSynthAddCB);
	eventsRegister(SISSM_EV_CLIENT_DEL_SYNTH, syncDataClientSynthDelCB);
	eventsRegister(SISSM_EV_CHAT, syncDataChatCB);
	//eventsRegister(SISSM_EV_SIGTERM, syncDataSigtermCB);
	//eventsRegister(SISSM_EV_WINLOSE, syncDataWinLose);
	//eventsRegister(SISSM_EV_TRAVEL, syncDataTravel);
	//eventsRegister(SISSM_EV_SESSIONLOG, syncDataSessionLog);
	//eventsRegister(SISSM_EV_OBJECT_SYNTH, syncDataObjectSynth);
	eventsRegister(SISSM_EV_KILLED, syncDataKilledCB);
	eventsRegister(SISSM_EV_SS_TAKE_OBJECTIVE, syncDataTakeObjectiveCB);
	//int j = 0;
	//rosterSetTestData(0, "PRC-Ranger™", "76561199022218080", "1.1.1.1","100");
	//rosterSetTestData(1, "灰灰·烬", "76561198846188569", "1.1.1.1", "100");

	//eventsDispatch("[2022.11.22-15.06.56:596][ 17]LogNet: Join succeeded: 灰灰·烬");
	//eventsDispatch("[2022.11.22-15.06.56:596][ 17]LogNet: Join succeeded: PRC-Ranger™");
 // readLogFile("C:\\3F-no-take-Insurgency.log");
 //readLogFile("C:\\3F-sync_data_Insurgency.log");
	//readLogFile("C:\\3F-Insurgency-backup-2022.11.22-22.59.56.log");
	//ReadFile("C:\\Users\\Administrator\\Desktop\\服务器\\sissm_src\\test-log.txt");
		//printf("========================playerList 0 =%s=%s=\n", playerList[0].name, playerList[0].uid);
		//printf("========================playerList 1 =%s=%s=\n", playerList[1].name, playerList[1].uid);
		//printf("========================playerDataIndex =%d\n", playerDataIndex);

		//// 1.一个玩家杀敌
		//char* log1 = "[2022.11.22 - 15.37.12:202][655]LogGameplayEvents: Display: Moonkidz[76561198049466120, team 1] killed Rifleman[INVALID, team 0] with BP_Firearm_M16A4_C_2147230188";
		//// 2.AI+玩家杀敌
		//char* log2 = "[2022.11.22-15.36.12:452][655]LogGameplayEvents: Display: Rifleman[INVALID, team 0] killed Moonkidz[76561198049466120, team 1] with BP_Firearm_M16A4_C_2147246503";
		//// 3.自杀
		//char* log3 = "[2022.11.22-15.36.34:422][894]LogGameplayEvents: Display: 璃月最強伝説と斩尽の牛杂!玉衡☆刻晴です[76561198971631233, team 1] killed 璃月最強伝説と斩尽の牛杂!玉衡☆刻晴です[76561198971631233, team 1] with BP_Character_Player_C_2147479854";
		//// 4.AI杀敌
		//char* log4 = "[2022.11.22-15.38.27:110][484]LogGameplayEvents: Display: Rifleman[INVALID, team 0] killed 头脑风暴[76561198091709844, team 1] with BP_Firearm_M4A1_C_2147228688";
		//// 5.多个玩家杀敌
		//char* log5 = "[2022.11.22-15.38.30:497][603]LogGameplayEvents: Display: PRC-Ranger™[76561199022218080, team 1] + Moonkidz[76561198049466120, team 1] killed Breacher[INVALID, team 0] with BP_Firearm_M60_C_2147230290";
		//// 6.爆破爆炸杀敌
		//char* log6 = "[2022.11.22-15.53.33:624][310]LogGameplayEvents: Display: 嘤嘤复嘤嘤[76561198119316406, team 0] killed Rifleman[INVALID, team 1] with ODCheckpoint_D\n";
		//char* log7 = "[2022.11.22-15.38.27:110][484]LogGameplayEvents: Display: PRC-Ranger™[76561199022218080, team 1] killed Rifleman[INVALID, team 1] with BP_Firearm_M4A1_C_2147228688";
		//// 玩家加入
		//char* playerGUID = "76561199022218080";
		//char* playerName = "PRC-Ranger™";
		//struct PlayerData* player = playerGetOrCreate(playerGUID);
		//strlcpy(player->name, playerName, sizeof(player->name));

		//struct PlayerData* player2 = playerGetOrCreate("76561198971631233");
		//strlcpy(player2->name, "璃月最強伝説と斩尽の牛杂!玉衡☆刻晴です", sizeof(player2->name));


		//eventsDispatch(log1);
		//eventsDispatch(log2);
		//eventsDispatch(log3);
		//eventsDispatch(log4);
		//eventsDispatch(log5);
		//eventsDispatch(log7);


		//// 1.爆破完成，其中Objective 0，0表示A点，以此类推
		//char* log20 = "[2022.11.22 - 16.56.30:228][783]LogGameplayEvents: Display: Objective 0 owned by team 1 was destroyed for team 0 by 嘤嘤复嘤嘤[76561198119316406], PRC - Ranger™[76561199022218080].";
		//// 2.占点完成
		//char* log21 = "[2022.11.22 - 15.18.43:017][790]LogGameplayEvents: Display: Objective 1 was captured for team 1 from team 0 by 璃月最強伝説と斩尽の牛杂!玉衡☆刻晴です[76561198971631233], 血战钢锯岭[76561198324874244], prtFrater[76561198360887006], ZJ1ah0[76561198230516704], 灰灰·烬[76561198846188569], littlewhite20[76561198215306701], CCA[76561198243423130].";
		//// 3.被AI夺回
		//char* log22 = "[2022.11.22-15.35.40:591][832]LogGameplayEvents: Display: Objective 0 owned by team 0 was reset from 36% by Rifleman[INVALID], Rifleman[INVALID], Rifleman[INVALID], Rifleman[INVALID], Rifleman[INVALID].";

		//eventsDispatch(log20);
		//eventsDispatch(log21);
		//eventsDispatch(log22);


		//char* log40 = "[2022.11.22-16.03.37:490][136]LogGameMode: Display: Round Over: Team 1 won (win reason: Elimination)";
		////syncDataWinLose(log40);
		//eventsDispatch(log40);


		/*char json[1000];
		createRoundJson(json);

		printf("json=%s\n", json);*/

		// if plugin is disabled in the .cfg file then do not activate
		//
		/*if (syncDataConfig.pluginState == 0)
			return 0;*/


			// todo 请求填写json

		//char* requestBody= "{\"roundData\":{\"serverId\":2,\"gameIsFull\":1,\"gameStartTime\":1669540543,\"gameRountLimit\":-1,\"roundId\":6,\"roundStartTime\":1669542487,\"roundEndTime\":1669542656,\"mapName\":\"Hideout_Checkpoint_Security\",\"win\":2,\"endReason\":\"Elimination\",\"takePosition\":\"1\"},\"playerList\":[{\"name\":\"血战钢锯岭\",\"uid\":\"76561198324874244\",\"score\":1705,\"killCount\":9,\"deadCount\":1,\"assistCount\":0,\"suicideCount\":0,\"takeCount\":0,\"joinTime\":1669542487,\"leaveTime\":0,\"ip\":\"123.123.47.80\",\"killWay\":{\"BP_Firearm_AUG\":7,\"BP_Projectile_Molotov\":1,\"BP_Firearm_PF940\":1},\"deadWay\":{\"BP_Firearm_Mosin\":1}},{\"name\":\"真主\",\"uid\":\"76561198257228155\",\"score\":2305,\"killCount\":3,\"deadCount\":1,\"assistCount\":1,\"suicideCount\":0,\"takeCount\":0,\"joinTime\":1669542487,\"leaveTime\":0,\"ip\":\"219.143.129.19\",\"killWay\":{\"BP_Firearm_M16A4\":3},\"deadWay\":{\"BP_Firearm_M16A4\":1}},{\"name\":\"青春Yuuki会梦到柯提羊\",\"uid\":\"76561198404297820\",\"score\":0,\"killCount\":0,\"deadCount\":0,\"assistCount\":0,\"suicideCount\":0,\"takeCount\":0,\"joinTime\":1669542605,\"leaveTime\":0,\"ip\":\"171.113.172.63\"}]}";
		//char responseBody[10 * 1024];
		//httpRequest(5000, 2, "82.156.36.121", "/api.php", requestBody, responseBody);
		////// todo 返回解析json
		//printf("responseBody=%s\n", responseBody);



	return 0;
}
