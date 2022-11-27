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

#include "map_vote.h"
#include <ctype.h>
#include "common_util.h"
//  ==============================================================================================
//  Data definition
//
static struct
{

	int pluginState; // always have this in .cfg file:  0=disabled 1=enabled
	int lessPlayerCountNeedAllVote;
	char allowDuplicateVoteUid[200];
	int duplicateVoteAsNewPlayer;
} mapVoteConfig;

#define MAP_COUNT 16
#define ROOM_PLAYER_MAX_COUNT 50

struct
{
	char name1[15];
	char name2[15];

} mapTable[] = {
	{"Prison", "Prison"},
	{"Bab", "Bab"},
	{"Ministry", "Ministry"},
	{"Citadel", "Citadel"},
	{"Crossing", "Canyon"},
	{"Farmhouse", "Farmhouse"},
	{"Gap", "Gap"},
	{"Hideout", "Town"},
	{"Hillside", "Sinjar"},
	{"Outskirts", "Compound"},
	{"Precinct", "Precinct"},
	{"Refinery", "Oilfield"},
	{"Summit", "Mountain"},
	{"PowerPlant", "PowerPlant"},
	{"Tell", "Tell"},
	{"Tideway", "Buhriz"} };

struct VoteResult
{
	int dayNum;
	int nightNum;
	int secNum;
	int insNum;
};

struct VoteResult voteResult[MAP_COUNT];

struct
{
	int index;
	char* day;
	char* sec;
} travelingMap[] = { -1, NULL, NULL };


struct PlayerVoteName
{
	char name[50];
	char uid[30];
};

struct PlayerVoteName votePlayers[ROOM_PLAYER_MAX_COUNT];

int mapState = 0;
int playerIndex = -1;
int useSeconds = 0;

//  ==============================================================================================
//  InitConfig
//
//  Read parameters from the .cfg file
//
int mapVoteInitConfig(void)
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
int mapVoteInitCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "map_vote", "Init Event ::%s::", strIn);
	return 0;
}

void mapVotePrintMap()
{
	char outStr[1024];
	// 使用strlcat之前保证变量至少包含一个	\0
	strclr(outStr);
	int i;
	for (i = 0; i < MAP_COUNT; i++)
	{
		char item[20];
		snprintf(item, 20, " [%d]%s", i + 1, mapTable[i].name1);
		strlcat(outStr, item, 1024);
	}

	logPrintf(LOG_LEVEL_INFO, "map_vote", "mapVoteInstallPlugin %s", outStr);
	apiSay(outStr);
	logPrintf(LOG_LEVEL_INFO, "map_vote", "test map %d %d %d", outStr[0], outStr[1], outStr[2]);
	apiSay("[选择地图]请输入序号投票: 序号后可带上字母:s,i,d,n,例如:6in");
	apiSay("[字母解释]:s=Security, i=Insurgents, d=Day, n=Night");
}

void startVote()
{
	if (mapState == 0)
	{

		int i;
		for (i = 0; i < MAP_COUNT; i++)
		{
			voteResult[i].dayNum = 0;
			voteResult[i].nightNum = 0;
			voteResult[i].secNum = 0;
			voteResult[i].insNum = 0;
		}

		playerIndex = -1;
		useSeconds = 0;
		mapState = 1;
	}

	mapVotePrintMap();
}

//  ==============================================================================================
//  GameEndCB
//
//  This callback is invoked whenever a Game-End event is detected.
//
int mapVoteGameEndCB(char* strIn)
{
	startVote();

	logPrintf(LOG_LEVEL_INFO, "map_vote", "Game End Event ::%s::", strIn);
	return 0;
}

//  ==============================================================================================
//  PeriodicCB
//
//  This callback is invoked at 1.0Hz rate (once a second).  Due to serial processing,
//  exact timing cannot be guaranteed.
//
int mapVotePeriodicCB(char* strIn)
{
	if (mapState == 0 && travelingMap->index < 0)
	{
		return 0;
	}

	useSeconds++;
	if (useSeconds == 15)
	{
		apiSay("距投票结束还有15秒钟,输入地图序号投票");
		return 0;
	}

	if (useSeconds == 30)
	{
		mapState = 0;
		int i;
		int maxIndex = -1;
		int maxNum = 0;
		int allVoteNum = 0;
		for (i = 0; i < MAP_COUNT; i++)
		{
			int currMapVoteNum = (voteResult[i].dayNum + voteResult[i].nightNum + voteResult[i].secNum + voteResult[i].insNum) / 2;

			if (currMapVoteNum > maxNum)
			{
				maxNum = currMapVoteNum;
				maxIndex = i;
			}
			allVoteNum += currMapVoteNum;
		}
		int votePlayerCount = 0;
		if (mapVoteConfig.duplicateVoteAsNewPlayer == 1) {
			// 每一次的重复的投票,都充当新的一名参与玩家
			votePlayerCount = allVoteNum;
		}
		else {
			// 真实的参与投票的玩家
			votePlayerCount = playerIndex + 1;
		}

		// 参与玩家大于总玩家的1/3		}
		int playersCount = apiPlayersGetCount();
		if (playersCount <= mapVoteConfig.lessPlayerCountNeedAllVote && votePlayerCount < playersCount) {
			apiSay("投票失败:原因是玩家少于%d人时需要全票(%d/%d)", mapVoteConfig.lessPlayerCountNeedAllVote, votePlayerCount, playersCount);
			return 0;
		}

		if (votePlayerCount < playersCount / 2.0)
		{

			apiSay("投票失败:原因是参与投票的人数少于50%(%d/%d)", votePlayerCount, playersCount);
			return 0;
		}

		if (maxIndex < 0)
		{
			apiSay("投票失败:无人投票");
			return 0;
		}

		travelingMap->index = maxIndex;
		travelingMap->day = voteResult[maxIndex].dayNum > voteResult[maxIndex].nightNum ? "Day" : "Night";
		travelingMap->sec = voteResult[maxIndex].secNum > voteResult[maxIndex].insNum ? "Security" : "Insurgents";

		logPrintf(LOG_LEVEL_INFO, "map_vote", "vote end; playersCount=%d  votePlayerCpunt=%d  maxIndex=%d,maxNum=%d day=%s,sec=%s map=%s", playersCount, votePlayerCount, maxIndex, maxNum, travelingMap->day, travelingMap->sec, mapTable[travelingMap->index].name1);
		apiSay("投票成功:共%d个玩家参与, 投票最多地图:%s [%s] [%s](%d次) ", votePlayerCount, mapTable[travelingMap->index].name1, travelingMap->day, travelingMap->sec, maxNum);
	}

	if (travelingMap->index >= 0 && useSeconds == 34)
	{ // ?Mutators=PistolsOnly?Game=Checkpoint?Mode=Checkpoint
		// BoltActionsOnly,SlowMovement,SlowCaptureTimes
		char strOut[300], strResp[300];
		snprintf(strOut, 300, "travel %s?Scenario=Scenario_%s_Checkpoint_%s?Lighting=%s", mapTable[travelingMap->index].name2, mapTable[travelingMap->index].name1, travelingMap->sec, travelingMap->day);
		travelingMap->index = -1;
		travelingMap->sec = NULL;
		travelingMap->day = NULL;
		apiRcon(strOut, strResp);
	}

	return 0;
}




void parseText(char* inputText, int* output)
{
	// 用前先复制，函数结束会释放掉局部变量
	char input[20];
	strlcpy(input, inputText, 20);


	int sec = 1;
	int day = 1;
	char number[3];
	int numberIndex = 0;

	int len = strlen(input);

	for (int i = 0; i < strlen(input); i++)
	{

		if (input[i] == 'i')
		{
			sec = 0;
		}
		else if (input[i] == 'n')
		{
			day = 0;
		}
		else if (input[i] >= '0' && input[i] <= '9')

		{
			if (numberIndex < 2)
			{
				number[numberIndex] = input[i];
				numberIndex++;
			}
			else

			{
				return;
			}
		}
		else if (input[i] == 's' || input[i] == 'd')
		{
			continue;
		}
		else
		{
			return;
		}
	}
	if (number[0] == 48)
	{
		return;
	}

	number[numberIndex] = '\0';

	int n = strtoi(number) - 1;

	if (n < 0)
	{
		return;
	}
	output[0] = n;
	output[1] = sec;
	output[2] = day;
}

//  ==============================================================================================
//  ChatCB
//
//  This callback is invoked whenever any player enters a text chat message.
//
int mapVoteChatCB(char* strIn)
{

	//logPrintf(LOG_LEVEL_INFO, "map_vote", "Client chat ::%s::", strIn);

	if (NULL != strstr(strIn, "vote") || NULL != strstr(strIn, "map"))
	{
		startVote();
		return 0;
	}

	if (mapState == 0)
	{
		return 0;
	}

	char name[50];
	char uid[30];
	getWordRange(strIn, "Display:", "(", name);
	getWordRange(strIn, "(", ")", uid);

	int allowDuplicateVoteUid = 0;
	if (strstr(mapVoteConfig.allowDuplicateVoteUid, uid) != NULL) {
		allowDuplicateVoteUid = 1;
	}

	int has = 0;
	int i;
	for (i = 0; i <= playerIndex; i++)
	{
		if (strcmp(votePlayers[i].uid, uid) == 0)
		{
			has = 1;
			break;
		}
	}

	logPrintf(LOG_LEVEL_INFO, "map_vote", "chat name=%s has=%d allowDuplicateVoteUid=%d", uid, has, allowDuplicateVoteUid);
	if (has == 1 && allowDuplicateVoteUid == 0)
	{
		return 0;
	}

	char textTemp[50], text[50];
	getWordRight(strIn, " Chat:", textTemp);

	trim(textTemp, text, sizeof(textTemp));
	if (strlen(text) == 0)
	{
		return 0;
	}

	logPrintf(LOG_LEVEL_INFO, "map_vote", "chat text=%s", text);
	int ret[3] = { -1, -1, -1 };
	parseText(text, ret);

	int mapIndex = ret[0];
	int mapSec = ret[1];
	int mapDay = ret[2];
	if (mapIndex >= 0)
	{
		char* secStr, * dayStr;
		if (mapSec == 1)
		{
			secStr = "Security";
			voteResult[mapIndex].secNum++;
		}
		else
		{
			secStr = "Insurgents";
			voteResult[mapIndex].insNum++;
		}

		if (mapDay == 1)
		{
			dayStr = "Day";
			voteResult[mapIndex].dayNum++;
		}
		else
		{
			dayStr = "Night";
			voteResult[mapIndex].nightNum++;
		}

		// add name to array
		if (has == 0 && playerIndex < ROOM_PLAYER_MAX_COUNT - 1)
		{
			++playerIndex;
			strlcpy(votePlayers[playerIndex].uid, uid, 30);
			strlcpy(votePlayers[playerIndex].name, name, 50);

			logPrintf(LOG_LEVEL_INFO, "map_vote", "list 0=%s", votePlayers[0].uid);
			if (playerIndex > 0) {
				logPrintf(LOG_LEVEL_INFO, "map_vote", "list 1=%s", votePlayers[1].uid);
			}
		}
		apiSay("[%s]投票: [%s] [%s] [%s]", name, mapTable[mapIndex].name1, secStr, dayStr);
	}
	logPrintf(LOG_LEVEL_INFO, "map_vote", "Client chat vote mapIndex=%d sec=%d day=%d", mapIndex, mapSec, mapDay);

	return 0;
}

//  ==============================================================================================
//  ...
//
//  ...
//  This method is exported and is called from the main sissm module.
//
int mapVoteInstallPlugin(void)
{
	logPrintf(LOG_LEVEL_INFO, "map_vote", "mapVoteInstallPlugin");

	mapVoteInitConfig();
	// if plugin is disabled in the .cfg file then do not activate
	//
	if (mapVoteConfig.pluginState == 0)
		return 0;

	// Install Event-driven CallBack hooks so the plugin gets
	// notified for various happenings.  A complete list is here,
	// but comment out what is not needed for your plug-in.
	//
	eventsRegister(SISSM_EV_INIT, mapVoteInitCB);
	eventsRegister(SISSM_EV_GAME_END_NOW, mapVoteGameEndCB);
	eventsRegister(SISSM_EV_PERIODIC, mapVotePeriodicCB);
	eventsRegister(SISSM_EV_CHAT, mapVoteChatCB);
	return 0;
}
