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

#include "suicide.h"
#include <ctype.h>
#include "common_util.h"
//  ==============================================================================================
//  Data definition
//
static struct
{

	int pluginState; // always have this in .cfg file:  0=disabled 1=enabled
	int limitCountPerRound;
} suicideConfig;

#define ROOM_PLAYER_MAX_COUNT 50
struct suicidePlayer
{
	char uid[20];
	int suicideCount;
};

struct suicidePlayer player[ROOM_PLAYER_MAX_COUNT];
int suicidePlayerIndex = -1;
//  ==============================================================================================
//  InitConfig
//
//  Read parameters from the .cfg file
//
int suicideInitConfig(void)
{
	cfsPtr cP;

	cP = cfsCreate(sissmGetConfigPath());

	// read config variable from the .cfg file
	suicideConfig.pluginState = (int)cfsFetchNum(cP, "suicide.pluginState", 0.0); // disabled by default

	suicideConfig.limitCountPerRound = (int)cfsFetchNum(cP, "suicide.limitCountPerRound", 2);
	cfsDestroy(cP);
	return 0;
}

//  ==============================================================================================
// InitCB
//
//  This callback is invoked whenever SISSM is started or reset.
//
int suicideInitCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "suicide", "Init Event ::%s::", strIn);
	return 0;
}


//  ==============================================================================================
//  GameEndCB
//
//  This callback is invoked whenever a Game-End event is detected.
//
int suicideGameEndCB(char* strIn)
{
	suicidePlayerIndex = -1;
	logPrintf(LOG_LEVEL_INFO, "suicide", "Game End Event ::%s::", strIn);
	return 0;
}

int suicideKilledCB(char* strIn)
{
	struct KillInfo* killInfo = NULL;
	killInfo = (struct KillInfo*)malloc(sizeof(*killInfo));
	parseKillInfo(strIn, killInfo);
	if (killInfo->playerIndex < 0|| !isValidUid(killInfo->deadUid)) {
		return 0;
	}

	char* playerUid = killInfo->playerUid[0];
	char* playerName = killInfo->playerName[0];

	int suicide = strcmp(playerUid, killInfo->deadUid) == 0 ? 1 : 0;


	if (suicide)
	{
		int has = 0;
		for (int i = 0; i <= suicidePlayerIndex; i++) {
			if (strcmp(player[i].uid, killInfo->deadUid) == 0)
			{
				has = 1;
				player[i].suicideCount++;

				if (player[i].suicideCount > suicideConfig.limitCountPerRound) {
					logPrintf(LOG_LEVEL_INFO, "suicide", "limit");
					apiKickOrBan(0, killInfo->deadUid, "自杀次数超限");
				}
				else {
					// fix encode add . end of
					apiSay("警告:[%s]已自杀%d次,超过%d次将被踢出.", killInfo->deadName, player[i].suicideCount, suicideConfig.limitCountPerRound);
				}
				break;
			}
		}


		if (has == 0) {
			++suicidePlayerIndex;
			strlcpy(player[suicidePlayerIndex].uid, killInfo->deadUid, sizeof(player[suicidePlayerIndex].uid));
			player[suicidePlayerIndex].suicideCount = 1;
			apiSay("警告:[%s]已自杀%d次,超过%d次将被踢出.", killInfo->deadName, player[suicidePlayerIndex].suicideCount, suicideConfig.limitCountPerRound);
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
int suicideInstallPlugin(void)
{
	logPrintf(LOG_LEVEL_INFO, "suicide", "suicideInstallPlugin");

	suicideInitConfig();
	// if plugin is disabled in the .cfg file then do not activate
	//
	if (suicideConfig.pluginState == 0)
		return 0;
	// Install Event-driven CallBack hooks so the plugin gets
	// notified for various happenings.  A complete list is here,
	// but comment out what is not needed for your plug-in.
	//
	eventsRegister(SISSM_EV_INIT, suicideInitCB);
	eventsRegister(SISSM_EV_GAME_END_NOW, suicideGameEndCB);
	eventsRegister(SISSM_EV_KILLED, suicideKilledCB);
	return 0;
}
