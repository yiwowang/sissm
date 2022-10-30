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

#include "kill_self.h"
#include <ctype.h>
//  ==============================================================================================
//  Data definition
//
static struct
{

	int pluginState; // always have this in .cfg file:  0=disabled 1=enabled
	int killSelfCount;
} killSelfConfig;

#define ROOM_PLAYER_MAX_COUNT 50
struct KillSelfPlayer
{
	char* name;
	int killSelfCount;
};

struct KillSelfPlayer player [ROOM_PLAYER_MAX_COUNT];
int playerIndex = -1;
//  ==============================================================================================
//  InitConfig
//
//  Read parameters from the .cfg file
//
int killSelfInitConfig(void)
{
	cfsPtr cP;

	cP = cfsCreate(sissmGetConfigPath());

	// read config variable from the .cfg file
	killSelfConfig.pluginState = (int)cfsFetchNum(cP, "kill_self.pluginState", 0.0); // disabled by default

	killSelfConfig.limitCountPerRound = (int)cfsFetchNum(cP, "kill_self.limitCountPerRound", 2);
	cfsDestroy(cP);
	return 0;
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
	if (NULL == (w2 = getWord(w, 0, end)))
	{
		return;
	}
	strlcpy(output, w2 + strlen(start), 100);
}
//  ==============================================================================================
// InitCB
//
//  This callback is invoked whenever SISSM is started or reset.
//
int killSelfInitCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "kill_self", "Init Event ::%s::", strIn);
	return 0;
}


//  ==============================================================================================
//  GameEndCB
//
//  This callback is invoked whenever a Game-End event is detected.
//
int killSelfGameEndCB(char* strIn)
{
	playerIndex = -1;
	logPrintf(LOG_LEVEL_INFO, "kill_self", "Game End Event ::%s::", strIn);
	return 0;
}

int killSelfKilledCB(char* strIn)
{
	logPrintf(LOG_LEVEL_INFO, "kill_self", "killSelfKilled Event ::%s::", strIn);
	char[100] name1;
	getWordRange(strIn, "Display:", "killed", name1);
	char[100] name2;
	getWordRange(strIn, "killed:", "with", name2);
	logPrintf(LOG_LEVEL_INFO, "kill_self", "name1::%s:: name2 %s", name1, name2);

	if (strncmp(name1, name2)==0) {
		int has = 0;
		int i;
		for (i = 0; i <= playerIndex; i++)
		{
			if (player[i] != NULL && strncmp(players[i].name, name1, strlen(name1)) == 0)
			{
				has = 1;
				if (player[i].killSelfCount > killSelfConfig.killSelfCount) {
					logPrintf(LOG_LEVEL_INFO, "kill_self", "limit");
				}
				break;
			}
		}

		if (has == 0) {
			++playerIndex;
			player[playerIndex].name = name1;
			player[playerIndex].killSelfCount = 1;
		}
		logPrintf(LOG_LEVEL_INFO, "kill_self", "has %d", has);
	}
	return 0;
}


//  ==============================================================================================
//  ...
//
//  ...
//  This method is exported and is called from the main sissm module.
//
int killSelfInstallPlugin(void)
{
	logPrintf(LOG_LEVEL_INFO, "kill_self", "killSelfInstallPlugin");

	killSelfInitConfig();
	// if plugin is disabled in the .cfg file then do not activate
	//
	if (killSelfConfig.pluginState == 0)
		return 0;

	// Install Event-driven CallBack hooks so the plugin gets
	// notified for various happenings.  A complete list is here,
	// but comment out what is not needed for your plug-in.
	//
	eventsRegister(SISSM_EV_INIT, killSelfInitCB);
	eventsRegister(SISSM_EV_GAME_END_NOW, killSelfGameEndCB);
	eventsRegister(SISSM_EV_KILLED, killSelfKilledCB);
	return 0;
}
