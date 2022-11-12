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
#include <http.h>

//  ==============================================================================================
//  Data definition
//
static struct
{
	int pluginState; // always have this in .cfg file:  0=disabled 1=enabled
	char * httpUrl;
	int joinShowOldName;
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
	KillWay killWay[10];// 
	KillWay beKillWay[10];// 
	int takeCount;
	long joinTime;
	long leaveTime;
	int score;
	char ip[15];

	char* oldName;
};

struct PlayerData playerData[ROOM_PLAYER_MAX_COUNT];


struct RoundData
{
	int serverIndex;
	int roundIndex;
	int maxRoundCount;
	long roundStartTime;
	long roundEndTime;
	int roundStartPlayerCount;
	int roundEndPlayerCount;
	char mapName[10];
	int win;
	char take[2];// 占领到哪个点了
};
struct RoundData roundData;
int playerIndex = -1;

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
	mapVoteConfig.pluginState = (int)cfsFetchNum(cP, "map_vote.pluginState", 0.0); // disabled by default
	mapVoteConfig.lessPlayerCountNeedAllVote = (int)cfsFetchNum(cP, "map_vote.lessPlayerCountNeedAllVote", 3);
	strlcpy(mapVoteConfig.allowDuplicateVoteUid, cfsFetchStr(cP, "map_vote.allowDuplicateVoteUid", ""), 200);
	mapVoteConfig.duplicateVoteAsNewPlayer = (int)cfsFetchNum(cP, "map_vote.duplicateVoteAsNewPlayer", 1);
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
	logPrintf(LOG_LEVEL_INFO, "stncData", "Init Event ::%s::", strIn);
	return 0;
}

int syncDataClientAddCB(char* strIn)
{
	char playerName[256], playerGUID[256], playerIP[256];

	rosterParsePlayerConn(strIn, 256, playerName, playerGUID, playerIP);
	logPrintf(LOG_LEVEL_INFO, "syncData", "Add Client ::%s::%s::%s::", playerName, playerGUID, playerIP);

	if (apiPlayersGetCount() >= 8) {   // if the last connect player is #8 (last slot if 8 port server)
		if (0 != strcmp(playerGUID, "76561000000000000")) {    // check if this is the server owner 
			apiKickOrBan(0, playerGUID, "Sorry the last port reserved for server owner only");
			apiSay("syncData: Player %s kicked server full", playerName);
		}
		apiSay("syncData: Welcome server owner %s", playerName);
	}
	else {
		apiSay("syncData: Welcome %s", playerName);
	}

	return 0;
}

int syncDataClientDelCB(char* strIn)
{
	char playerName[256], playerGUID[256], playerIP[256];

	rosterParsePlayerDisConn(strIn, 256, playerName, playerGUID, playerIP);
	logPrintf(LOG_LEVEL_INFO, "syncData", "Del Client ::%s::%s::%s::", playerName, playerGUID, playerIP);

	apiSay("syncData: Good bye %s", playerName);

	return 0;
}
int syncDataGameStartCB(char* strIn)
{
	char newCount[256];

	logPrintf(LOG_LEVEL_INFO, "syncData", "Game Start Event ::%s::", strIn);

	apiGameModePropertySet("minimumenemies", "4");
	apiGameModePropertySet("maximumenemies", "4");

	strlcpy(newCount, apiGameModePropertyGet("minimumenemies"), 256);

	// in-game announcement start of game
	apiSay("syncData: %s -- 2 waves of %s bots", syncDataConfig.stringParameterExample, newCount);

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
	apiSay(syncDataConfig.stringParameterExample2); // in-game announcement start of round
	return 0;
}

//  ==============================================================================================
//  syncDataRoundEndCB
//
//  This callback is invoked whenever a End-of-round event is detected.
//
int syncDataRoundEndCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "syncData", "Round End Event ::%s::", strIn);
	apiSay("syncData: End of Round");
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

		apiSay(syncDataConfig.stringParameterExample3);
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
	getWordRange1(strIn, "Display:", "killed", fullName1);
	char fullName2[100];
	getWordRange1(strIn, "killed", "with", fullName2);
	logPrintf(LOG_LEVEL_INFO, "kill_self", "name1::%s:: name2 %s", fullName1, fullName2);



	char name1[100];
	char uid1[40];
	getWordRange1(fullName1, " ", "[", name1);
	getWordRange1(fullName1, "[", ",", uid1);


	char name2[100];
	char uid2[40];
	getWordRange1(fullName2, " ", "[", name2);
	getWordRange1(fullName2, "[", ",", uid2);

}
//  ==============================================================================================
//  ...
//
//  ...
//  This method is exported and is called from the main sissm module.
//
int syncDataInstallPlugin(void)
{
	logPrintf(LOG_LEVEL_INFO, "map_vote", "mapVoteInstallPlugin");

	syncDataInitConfig();
	// if plugin is disabled in the .cfg file then do not activate
	//
	if (syncDataConfig.pluginState == 0)
		return 0;

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
httpRequest();
	return 0;
}
