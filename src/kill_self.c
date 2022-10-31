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
	int limitCountPerRound;
} killSelfConfig;

#define ROOM_PLAYER_MAX_COUNT 50
struct KillSelfPlayer
{
	char* name;
	int killSelfCount;
};

struct KillSelfPlayer player [ROOM_PLAYER_MAX_COUNT];
int playerIndex1 = -1;
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
void getWordRange1(char* input, char* start, char* end, char* output)
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


	strlcpy(output, w + strlen(start), w2-w-strlen(start)+1);
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
	playerIndex1 = -1;
	logPrintf(LOG_LEVEL_INFO, "kill_self", "Game End Event ::%s::", strIn);
	return 0;
}

int killSelfKilledCB(char* strIn)
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
 getWordRange1(fullName1," ","[",name1);
  getWordRange1(fullName1,"[",",",uid1);


   char name2[100];
 char uid2[40];
 getWordRange1(fullName2," ","[",name2);
  getWordRange1(fullName2,"[",",",uid2);

 logPrintf(LOG_LEVEL_INFO, "kill_self", "name1==%s==uid1==%s==%s==%s==", name1, uid1,name2,uid2);

		int i;
if ( strncmp(uid1, uid2, strlen(uid1)) == 0)
{		
for (i = 0; i <= playerIndex1; i++){
		if ( strncmp(player[i].name, uid1, strlen(uid1)) == 0)
		{
				has = 1;
					player[playerIndex1].killSelfCount++;
				
			if (player[i].killSelfCount > killSelfConfig.limitCountPerRound) {
					logPrintf(LOG_LEVEL_INFO, "kill_self", "limit");


					apiKickOrBan(0,uid1,"自杀次数超限");			
				}
			else{
				apiSay("警告:[%s]已自杀%d次,超过%d次将被踢出",name1, player[playerIndex1].killSelfCount,killSelfConfig.limitCountPerRound);
			}	
				break;
		}
}


		if (has == 0) {
			++playerIndex1;
			player[playerIndex1].name = uid1;
			player[playerIndex1].killSelfCount = 1;
     apiSay("警告:[%s]已自杀%d次,超过%d次将被踢出",name1, player[playerIndex1].killSelfCount,killSelfConfig.limitCountPerRound);
		
}
		logPrintf(LOG_LEVEL_INFO, "kill_self", "has %d,==%s==%s==", has,name1,uid1);
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


char * strIn="[2022.10.28-01.28.41:552][383]LogGameplayEvents: Display: 血战钢锯岭[76561198324874244, team 1] killed 血战钢锯岭[76561198324874244, team 1] with BP_Projectile_Molotov_C_2147480221";

	logPrintf(LOG_LEVEL_INFO, "kill_self", "killSelfInstallPlugin");
         char fullName1[100];
         getWordRange1(strIn, "Display:", "killed", fullName1);
         char fullName2[100];
         getWordRange1(strIn, "killed", "with", fullName2);
         logPrintf(LOG_LEVEL_INFO, "kill_self", "name1::%s:: name2 %s", fullName1, fullName2);



  char name1[100];
 char uid1[40];
 getWordRange1(fullName1," ","[",name1);
  getWordRange1(fullName1,"[",",",uid1);


   char name2[100];
 char uid2[40];
 getWordRange1(fullName2," ","[",name2);
  getWordRange1(fullName2,"[",",",uid2);

 logPrintf(LOG_LEVEL_INFO, "kill_self", "name1==%s==uid1==%s==%s==%s==", name1, uid1,name2,uid2);

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
