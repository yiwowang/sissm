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
	char* httpUrl;
	int serverId;
} syncDataConfig;

#define MAP_COUNT 16
#define ROOM_PLAYER_MAX_COUNT 20

struct KillWay {
	char  weaponName[20];
	int killCount;
};

 struct PlayerData
{
	char name[30];// 玩家名字
	char uid[20];// 玩家steamId
	int score;// 玩家得分
	char ip[15];// ip
	struct KillWay killWay[50];// 杀敌的武器类型和数量
	struct KillWay beKillWay[50];// 被杀的武器类型和数量
	int takeCount;// 站点数量
	long joinTime;// 加入时间
	long leaveTime;// 离开时间

	int killWayIndex;
	int beKillWayIndex;
	char* oldName;// 上次用的名字
};

struct PlayerData  playerList[50];
int playerDataIndex = -1;
int gameStartTime;
int gameRountIndex;
struct RoundData
{
	int serverIndex;// 几服
	long gameStartTime;//此轮开始时间
	int roundIndex;// 第几局
	int maxRoundCount;// 最大几局
	long roundStartTime;
	long roundEndTime;
	int roundStartPlayerCount;
	int roundEndPlayerCount;
	char mapName[10];
	int win;
	char take[2];// 占领到哪个点了
};
struct RoundData roundData;


void roundInit() {
	for (int i = 0; i <= playerDataIndex; i++) {
		playerList[i].score = 0;
		playerList[i].joinTime = 0;
		playerList[i].leaveTime = 0;
		playerList[i].takeCount = 0;
	}
	playerDataIndex = -1;

}

struct PlayerData*  playerGet(char* uid) {
	for (int i = 0; i <= playerDataIndex; i++) {
		if (strcmp(playerList[i].uid, uid)==0) {
			return &playerList[i];
		}
	}
	return NULL;
}

struct PlayerData*  playerGetOrCreate(char* uid) {
	struct PlayerData*  item = playerGet(uid);
	if (item == NULL) {
		item = &playerList[++playerDataIndex];
		strcpy(item->uid, uid);
		
	}
	return item;
}

int createRoundJson(char* strJson) {
	cJSON* pRoot = cJSON_CreateObject();

	cJSON* pRound = cJSON_CreateObject();
	/*char serverIndex[20];
	snprintf(serverIndex, 20, "%d", roundData.serverIndex);
	scanf("%d", &serverIndex);*/
	cJSON_AddNumberToObject(pRound, "serverIndex", roundData.serverIndex);
	cJSON_AddNumberToObject(pRound, "roundIndex", roundData.roundIndex);
	cJSON_AddNumberToObject(pRound, "maxRoundCount", roundData.maxRoundCount);
	cJSON_AddNumberToObject(pRound, "roundStartTime", roundData.roundStartTime);
	cJSON_AddNumberToObject(pRound, "roundEndTime", roundData.roundEndTime);
	cJSON_AddNumberToObject(pRound, "roundStartPlayerCount", roundData.roundStartPlayerCount);
	cJSON_AddNumberToObject(pRound, "roundEndPlayerCount", roundData.roundEndPlayerCount);
	cJSON_AddStringToObject(pRound, "mapName", roundData.mapName);
	cJSON_AddNumberToObject(pRound, "win", roundData.win);
	cJSON_AddStringToObject(pRound, "take", roundData.take);
	cJSON_AddItemToObject(pRoot, "roundData", pRound);

	cJSON* pArray = cJSON_CreateArray();
	int i;
	for (i = 0; i <= playerDataIndex; i++) {
		cJSON* pItem = cJSON_CreateObject();
		cJSON_AddStringToObject(pItem, "name", playerList[i].name);
		cJSON_AddStringToObject(pItem, "uid", playerList[i].uid);
		cJSON_AddNumberToObject(pItem, "score", playerList[i].score);

		cJSON* pKillWayArray = cJSON_CreateArray();
		int j;
		for (j = 0; j < playerList[j].killWayIndex; j++) {
			cJSON* pKillWayItem = cJSON_CreateObject();
			cJSON_AddStringToObject(pKillWayItem, "weaponName", playerList[i].killWay[j].weaponName);
			cJSON_AddNumberToObject(pKillWayItem, "killCount", playerList[i].killWay[j].killCount);
			cJSON_AddItemToArray(pKillWayArray, pKillWayItem);
		}
		cJSON_AddItemToObject(pItem, "killWay", pKillWayArray);

		cJSON* pBeKillWayArray = cJSON_CreateArray();
		for (j = 0; j < playerList[j].beKillWayIndex; j++) {
			cJSON* pBeKillWayItem = cJSON_CreateObject();
			cJSON_AddStringToObject(pBeKillWayItem, "weaponName", playerList[i].beKillWay[j].weaponName);
			cJSON_AddNumberToObject(pBeKillWayItem, "killCount", playerList[i].beKillWay[j].killCount);
			cJSON_AddItemToArray(pBeKillWayArray, pBeKillWayItem);
		}
		cJSON_AddItemToObject(pItem, "beKillWay", pBeKillWayArray);

		cJSON_AddItemToArray(pArray, pItem);
	}

	cJSON_AddItemToObject(pRoot, "playerList", pArray);

	char* szJSON = cJSON_Print(pRoot);//通过cJSON_Print获取cJSON结构体的字符串形式（注：存在\n\t）
	//printf(szJSON);
	strlcpy(strJson, szJSON, strlen(szJSON));

	cJSON_Delete(pRoot);

	free(szJSON);
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
	syncDataConfig.serverId = (int)cfsFetchNum(cP, "map_vote.lessPlayerCountNeedAllVote", 3);
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

int syncDataClientAddCB(char* strIn)
{
	char playerName[256], playerGUID[256], playerIP[256];

	rosterParsePlayerConn(strIn, 256, playerName, playerGUID, playerIP);
	logPrintf(LOG_LEVEL_INFO, "syncData", "Add Client ::%s::%s::%s::", playerName, playerGUID, playerIP);
	struct PlayerData * player=playerGetOrCreate(playerGUID);
	strcpy(player->name, playerName);
	strcpy(player->ip, playerIP);
	player->joinTime= apiTimeGet();
	//if (apiPlayersGetCount() >= 8) {   // if the last connect player is #8 (last slot if 8 port server)
	//	if (0 != strcmp(playerGUID, "76561000000000000")) {    // check if this is the server owner 
	//		apiKickOrBan(0, playerGUID, "Sorry the last port reserved for server owner only");
	//		apiSay("syncData: Player %s kicked server full", playerName);
	//	}
	//	apiSay("syncData: Welcome server owner %s", playerName);
	//}
	//else {
	//	apiSay("syncData: Welcome %s", playerName);
	//}

	return 0;
}

int syncDataClientDelCB(char* strIn)
{
	char playerName[256], playerGUID[256], playerIP[256];

	rosterParsePlayerDisConn(strIn, 256, playerName, playerGUID, playerIP);
	logPrintf(LOG_LEVEL_INFO, "syncData", "Del Client ::%s::%s::%s::", playerName, playerGUID, playerIP);

	struct PlayerData * player = playerGetOrCreate(playerGUID);
	strcpy(player->name, playerName);
	strcpy(player->ip, playerIP);
	player->leaveTime = apiTimeGet();
	return 0;
}
int syncDataGameStartCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "syncData", "Game Start Event ::%s::", strIn);

	// in-game announcement start of game
	gameStartTime = apiTimeGet();
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
	return 0;
}


int syncDataRoundStartCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "syncData", "Round Start Event ::%s::", strIn);
	roundInit();
	roundData.gameStartTime = gameStartTime;
	roundData.roundIndex = gameRountIndex++;
	roundData.roundStartTime = apiTimeGet();
	return 0;
}

//  ==============================================================================================
//  syncDataRoundEndCB
//
//  This callback is invoked whenever a End-of-round event is detected.
//
int syncDataRoundEndCB(char* strIn)
{
	roundData.roundEndTime = apiTimeGet();
	logPrintf(LOG_LEVEL_INFO, "syncData", "Round End Event ::%s::", strIn);
	/*list_each(playerList, value)
	{
	}*/
	for (int i = 0; i < playerDataIndex; i++) {
		playerList[i].score = strtoi(rosterLookupIPFromGUID(playerList[i].uid),10);
	}

	// 发送
	return 0;
}

int syncDataWinLose(char* strIn)
{
	int isTeam0, humanSide;
	char outStr[256];

	humanSide = rosterGetCoopSide();
	isTeam0 = (NULL != strstr(strIn, "Team 0"));

	switch (humanSide) {
	case 0:
		if (!isTeam0) strlcpy(outStr, "Co-op Humans Win", 256);
		break;
	case 1:
		if (isTeam0) strlcpy(outStr, "Co-opHumans Lose", 256);
		break;
	default:
		strlcpy(outStr, "PvP WinLose", 256);
		break;
	}

	apiSay("syncData: %s", outStr);
	logPrintf(LOG_LEVEL_INFO, "syncData", outStr);

	return 0;
}


int syncDataCapturedCB(char* strIn)
{
	static unsigned long int lastTimeCaptured = 0L;
	// logPrintf( LOG_LEVEL_INFO, "syncData", "Captured Objective Event ::%s::", strIn );
	logPrintf(LOG_LEVEL_INFO, "syncData", "Captured Objective Event");

	// system generates multiple 'captured' report, so it is necessary
	// to add a 10-second window filter to make sure only one gets fired off.
	//
	if (lastTimeCaptured + 10L < apiTimeGet()) {

		lastTimeCaptured = apiTimeGet();
		// apiServerRestart();

	}

	return 0;
}

int syncDataChatCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "syncData", "Client chat ::%s::", strIn);
	return 0;
}

int syncDataKilledCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "kill_self", "killSelfKilled Event ::%s::", strIn);

	int has = 0;
	char fullName1[100];
	getWordRange(strIn, "Display:", "killed", fullName1);
	char fullName2[100];
	getWordRange(strIn, "killed", "with", fullName2);
	logPrintf(LOG_LEVEL_INFO, "kill_self", "name1::%s:: name2 %s", fullName1, fullName2);



	char name1[100];
	char uid1[40];
	getWordRange(fullName1, " ", "[", name1);
	getWordRange(fullName1, "[", ",", uid1);


	char name2[100];
	char uid2[40];
	getWordRange(fullName2, " ", "[", name2);
	getWordRange(fullName2, "[", ",", uid2);
	if (strlen(uid1)==0) {
		struct KillWay  way;
		way.killCount = 0;
		struct PlayerData* player = playerGet(uid2);
		// 被杀死的方式
		player->beKillWayIndex++;
		player->beKillWay[player->beKillWayIndex] = way;

	}
	else {
		struct KillWay  way;
		struct PlayerData* player = playerGet(uid1);
		way.killCount = 0;
		// 杀死的方式
		player->killWayIndex++;
		player->killWay[player->killWayIndex] = way;
	}
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
	// if plugin is disabled in the .cfg file then do not activate
	//
	/*if (syncDataConfig.pluginState == 0)
		return 0;*/


	// todo 请求填写json
	char responseBody[2048];
	httpRequest(5000,2,"82.156.36.121","/","{}", responseBody);
	// todo 返回解析json
	printf("responseBody=\n%s", responseBody);



	/*syncDataRoundStartCB(SS_SUBSTR_ROUND_START);
	syncDataChatCB("[2022.06.16-18.07.05:877][751]LogChat: Display: 血战钢锯岭(76561198324874244) Global Chat: vite");
	syncDataRoundEndCB(SS_SUBSTR_ROUND_END);*/

	struct PlayerData* p=playerGetOrCreate("1234");
	printf("=====uid %s\n", p->uid);
	printf("=====playerDataIndex %d\n", playerDataIndex);
	printf("=====playerData.uid %s\n", playerList[playerDataIndex].uid);

	struct PlayerData* p1 = playerGetOrCreate("1234");
	strcpy(p1->name,"张三");
	printf("=====playerData.name %s\n", playerList[playerDataIndex].name);
	char json[500];
	createRoundJson(json);

	printf("=====json %s\n", json);

	// Install Event-driven CallBack hooks so the plugin gets
	// notified for various happenings.  A complete list is here,
	// but comment out what is not needed for your plug-in.
	//
	eventsRegister(SISSM_EV_CLIENT_ADD, syncDataClientAddCB);
	eventsRegister(SISSM_EV_CLIENT_DEL, syncDataClientDelCB);
	eventsRegister(SISSM_EV_INIT, syncDataInitCB);
	//eventsRegister(SISSM_EV_RESTART, syncDataRestartCB);
	//eventsRegister(SISSM_EV_MAPCHANGE, syncDataMapChangeCB);
	eventsRegister(SISSM_EV_GAME_START, syncDataGameStartCB);
	eventsRegister(SISSM_EV_GAME_END_NOW, syncDataGameEndCB);
	eventsRegister(SISSM_EV_ROUND_START, syncDataRoundStartCB);
	eventsRegister(SISSM_EV_ROUND_END, syncDataRoundEndCB);
	eventsRegister(SISSM_EV_OBJECTIVE_CAPTURED, syncDataCapturedCB);
	//eventsRegister(SISSM_EV_PERIODIC, syncDataPeriodicCB);
	//eventsRegister(SISSM_EV_SHUTDOWN, syncDataShutdownCB);
	//eventsRegister(SISSM_EV_CLIENT_ADD_SYNTH, syncDataClientSynthAddCB);
	//eventsRegister(SISSM_EV_CLIENT_DEL_SYNTH, syncDataClientSynthDelCB);
	eventsRegister(SISSM_EV_CHAT, syncDataChatCB);
	//eventsRegister(SISSM_EV_SIGTERM, syncDataSigtermCB);
	eventsRegister(SISSM_EV_WINLOSE, syncDataWinLose);
	//eventsRegister(SISSM_EV_TRAVEL, syncDataTravel);
	//eventsRegister(SISSM_EV_SESSIONLOG, syncDataSessionLog);
	//eventsRegister(SISSM_EV_OBJECT_SYNTH, syncDataObjectSynth);
	eventsRegister(SISSM_EV_KILLED, syncDataKilledCB);
	return 0;
}
