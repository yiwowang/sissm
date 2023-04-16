#!/usr/bin/env python
# -*- coding: utf-8 -*
import json

from event import *
from requester import Requester
from socket_bridge import SocketWorker

socketWorker = None
requester = None


class TestPlugin(EventCallback):
    def roundEnd(self, log, data):
        print("TestPlugin roundStart 接收" + str(data))
       # requester.apiGameModePropertySet("MinimumEnemies", "4")
        # print("TestPlugin roundStart MinimumEnemies=1设置完毕")
        v = requester.apiGameModePropertyGet("MinimumEnemies")
        print("TestPlugin 获取MinimumEnemies值为" + str(v))

        v = requester.apiLookupLastObjectiveLetterFromCache()
        print("TestPlugin apiLookupLastObjectiveLetterFromCache" + str(v))

        requester.apiGameModePropertySet("bForceSoloWaves", "True")
        requester.apiGameModePropertySet("SoloReinforcementTime", "60")
        requester.apiGameModePropertySet("SoloWaveIncrementOnObjectiveCapture", "1")


if __name__ == "__main__":
    socketWorker = SocketWorker()
    requester = Requester(socketWorker)

    plugin = TestPlugin()
    eventDispatcher.register(plugin)
    eventDispatcher.init(requester)

    # msg = '{"event_type": "clientAdd", "log": "[2023.04.08-00.53.52:441][679]LogNet: Join succeeded: 测试账号","playerName": "测试账号", "playerGUID": "76561199275670546", "playerIP": "125.33.218.132"}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # msg = '{"event_type": "clientDel", "log": "[2023.04.08-01.54.23:797][356]LogNet: UChannel::Close: Sending CloseBunch. ChIndex == 0. Name: [UChannel] ChIndex: 0, Closing: 0 [UNetConnection] RemoteAddr: 125.33.218.132:17295, Name: IpConnection_2147481227, Driver: GameNetDriver IpNetDriver_2147482542, IsServer: YES, PC: INSPlayerController_2147481223, Owner: INSPlayerController_2147481223, UniqueId: SteamNWI:76561199275670546"}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # msg = '{"event_type": "gameStart","log": "[2023.04.08-00.56.05:070][603]LogGameState: Match State Changed from GameStarting to PreRound"}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # msg = '{"event_type": "roundStart","log": "[2023.04.08-00.56.11:079][959]LogGameState: Match State Changed from PreRound to RoundActive"}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # msg = '{"event_type": "roundStateChange","log": "[2023.04.08-00.56.11:079][959]LogGameplayEvents: Display: Round 1 started"}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # msg = '{"event_type":"winLose","log":"[2023.04.08-01.01.14:079][ 63]LogGameMode: Display: Round Over: Team 1 won (win reason: TimeExpired_DefendersPreventedFullCapture)", "humanSide":1}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # msg = '{"event_type": "objectSynth","log": "~SYNTHOBJ~ Obj_WeaponCache_Ins_C /Game/Maps/Precinct/Precinct_Checkpoint_Security Precinct_Checkpoint_Security PersistentLevel ODCheckpoint_B","obj": "ODCheckpoint_B", "typ": "Obj_WeaponCache_Ins_C"}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # 爆破完成，其中Objective
    # 表示A点，以此类推
    # msg= '{"event_type": "takeObject","log":"[2022.11.22 - 16.56.30:228][783]LogGameplayEvents: Display: Objective 0 owned by team 1 was destroyed for team 0 by yyyy[76561198119316406]."}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # # #占点完成
    # msg = '{"event_type": "takeObject","log":"[2022.11.22 - 15.18.43:017][790]LogGameplayEvents: Display: Objective 1 was captured for team 1 from team 0 by huanqina[76561198971631233], mee[76561198324874244], prtFrater[76561198360887006], ZJ1ah0[76561198230516704], huijin[76561198846188569], littlewhite20[76561198215306701], CCA[76561198243423130]."}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # #被AI夺回
    # msg = '{"event_type": "takeObject","log":"[2022.11.22-15.35.40:591][832]LogGameplayEvents: Display: Objective 0 owned by team 0 was reset from 36% by Rifleman[INVALID], Rifleman[INVALID], Rifleman[INVALID], Rifleman[INVALID], Rifleman[INVALID]."}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # msg = '{"event_type": "killed","log": "[2023.04.08-01.00.59:876][215]LogGameplayEvents: Display: 测试账号[76561199275670546, team 0] killed Breacher[INVALID, team 1] with BP_Firearm_AUG_C_2147480743"}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # msg = '{"event_type": "captured","log": "[2023.04.08-01.35.41:616][333]LogSpawning: Spawnzone \'SZCheckpoint_B_0\' enabled for team 0"}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # msg = '{"event_type": "clientSynthDel", "log": "~SYNTHDEL~ 76561198126974066 114.105.123.153 Jsen","playerName": "Jsen", "playerGUID": "76561198126974066", "playerIP": "114.105.123.153"}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # msg = '{"event_type": "clientSynthAdd", "log": "~SYNTHADD~ 76561198126974066 114.105.123.153 Jsen","playerName": "Jsen", "playerGUID": "76561198126974066", "playerIP": "114.105.123.153"}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # msg = '{"event_type": "chat","log": "[2023.04.08-01.07.26:712][325]LogChat: Display: Jsen(76561198126974066) Global Chat: da bu liao zhong wen"}'
    # jsonObject = json.loads(msg, strict=False)
    # eventDispatcher.dispatch(jsonObject)
    # exit(0)

    socketWorker.socketLoop()
