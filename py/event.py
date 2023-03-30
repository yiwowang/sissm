 #!/usr/bin/env python
 # -*- coding: utf-8 -*

SS_SUBSTR_ROUND_START = "Match State Changed from PreRound to RoundActive"
SS_SUBSTR_ROUND_END = "Match State Changed from RoundWon to PostRound"
SS_SUBSTR_GAME_START = "Match State Changed from GameStarting to PreRound"
SS_SUBSTR_GAME_END = "Match State Changed from GameOver to LeavingMap"
SS_SUBSTR_REGCLIENT = "LogNet: Join succeeded:"
SS_SUBSTR_UNREGCLIENT = "LogNet: UChannel::Close:"
SS_SUBSTR_OBJECTIVE = "LogGameMode: Display: Advancing spawns for faction"
SS_SUBSTR_MAPCHANGE = "SeamlessTravel to:"
SS_SUBSTR_CAPTURE = "LogSpawning: Spawnzone '"
SS_SUBSTR_SHUTDOWN = "LogExit: Game engine shut down"
SS_SUBSTR_CHAT = "LogChat: Display:"
SS_SUBSTR_WINLOSE = "LogGameMode: Display: Round Over: Team"
SS_SUBSTR_TRAVEL = "LogGameMode: ProcessServerTravel:"
SS_SUBSTR_SESSIONLOG = "HttpStartUploadingFinished. SessionName:"
SS_SUBSTR_DESTROY = "was destroyed for team"
SS_SUBSTR_RCON = "LogRcon:"

SS_SUBSTR_BP_PLAYER_CONN = " RestartPlayerAtPlayerStart "
SS_SUBSTR_BP_PLAYER_DISCONN = " Unpossessed"
SS_SUBSTR_BP_CHARNAME = "' cached new pawn '"
SS_SUBSTR_BP_TOUCHED_OBJ = " entered."
SS_SUBSTR_BP_UNTOUCHED_OBJ = " exited."

SS_SUBSTR_MAP_OBJECTIVE = "LogObjectives: Verbose: Authority: Adding objective '"
SS_SUBSTR_MESHERR = "LogGameMode: Verbose: RestartPlayerAt"
SS_SUBSTR_GAME_END_NOW = "LogGameMode: Display: State: WaitingPostMatch -> GameOver"


# todo 基于sissm，后面有必要再自己track
# 完全自己解析吧
class PlayerBean:
    playerName = ""
    playerGUID = ""
    playerIP = ""

    def parse(self, jsonObj):
        if jsonObj is not None:
            if "playerName" in jsonObj:
                self.playerName = jsonObj["playerName"]
            if "playerGUID" in jsonObj:
                self.playerGUID = jsonObj["playerGUID"]
            if "playerIP" in jsonObj:
                self.playerIP = jsonObj["playerIP"]
        return self


class EventCallback:
    requester = None

    def init(self, requester):
        self.requester = requester

    def clientAdd(self, log, player):
        pass

    def clientDel(self, log, player):
        pass

    def restart(self):
        pass

    def mapChange(self, log, mapName=""):
        pass

    def gameStart(self, log):
        pass

    def gameEnd(self, log):
        pass

    def roundStart(self, log):
        pass

    def roundEnd(self, log):
        pass

    def captured(self, log):
        pass

    def shutdown(self, log):
        pass

    def clientSynthDel(self, log, player):
        pass

    def clientSynthAdd(self, log, player):
        pass

    def chat(self, log):
        pass

    def sigterm(self, log):
        pass

    def winLose(self, log, win):
        pass

    def travel(self, log, mapName, scenario, mutator, humanSide):
        pass

    def sessionLog(self, log):
        pass

    def objectSynth(self, log):
        pass


class EventDispatcher:
    callbackList = []
    requester = None

    def init(self, requester):
        self.requester = requester
        if self.callbackList is not None:
            for callback in self.callbackList:
                callback.init(requester)

    def register(self, callback):
        self.callbackList.append(callback)

    def dispatch(self, log, jsonObject):
        for cb in self.callbackList:
            eventType = jsonObject["event_type"]
            if eventType == "clientAdd":
                cb.clientAdd(log, PlayerBean().parse(jsonObject))
            elif eventType == "clientDel":
                cb.clientDel(log, PlayerBean().parse(jsonObject))
            elif eventType == "restart":
                cb.restart(log)
            elif eventType == "mapChange":
                cb.mapChange(log, jsonObject["mapName"])
            elif eventType == "gameStart":
                cb.gameStart(log)
            elif eventType == "gameEnd":
                cb.gameEnd(log)
            elif eventType == "roundStart":
                cb.roundStart(log)
            elif eventType == "roundEnd":
                cb.roundEnd(log)
            elif eventType == "captured":
                cb.captured(log)
            elif eventType == "shutdown":
                cb.shutdown(log)
            elif eventType == "clientSynthDel":
                cb.clientSynthDel(log, PlayerBean().parse(jsonObject))
            elif eventType == "clientSynthAdd":
                cb.clientSynthAdd(log, PlayerBean().parse(jsonObject))
            elif eventType == "chat":
                cb.chat(log)
            elif eventType == "sigterm":
                cb.sigterm(log)
            elif eventType == "winLose":
                cb.winLose(log)
            elif eventType == "travel":
                cb.travel(log)
            elif eventType == "sessionLog":
                cb.sessionLog(log)
            elif eventType == "objectSynth":
                cb.objectSynth(log)


eventDispatcher = EventDispatcher()
