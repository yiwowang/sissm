# !/usr/bin/env python
# -*- coding: utf-8 -*
import json

import requests as requests

from lib.event_callback import EventCallback


class JoinMsgPlugin(EventCallback):
    requester = None
    configReader = None
    logger = None
    callbacks = []
    host = "127.0.0.1"

    def init(self, requester, configReader, logger):
        self.requester = requester
        self.configReader = configReader
        self.logger = logger

    def clientSynthAdd(self, log, data):
        print("JoinMsgPlugin clientSynthAdd 接收" + str(data))
        playerGUID = data.get("playerGUID")
        playerName = data.get("playerName")
        if playerGUID is None:
            return
        r = requests.get('http://' + self.host + '/insurgency/player_join_msg.php',
                         "playerGUID=" + playerGUID + "&playerName=" + playerName)
        dataStr = str(r.text)
        data = json.loads(dataStr)
        if data["code"] == 0 and len(data["data"]) > 0:
            self.requester.apiSay(str(data["data"]))
        else:
            print(dataStr)

    def roundStateChange(self, log, data):
        print("JoinMsgPlugin roundStateChange 接收" + str(data))
