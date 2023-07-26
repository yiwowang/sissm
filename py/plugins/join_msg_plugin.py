# !/usr/bin/env python
# -*- coding: utf-8 -*
import json
from lib.jianti_fanti import Translate

import sys
 
reload(sys)
sys.setdefaultencoding('utf8')
import requests as requests

from lib.event_callback import EventCallback


translate = Translate()
tw = translate.ToTraditionalChinese("我爱中国")
print("aa "+tw)
class JoinMsgPlugin(EventCallback):
    requester = None
    configReader = None
    logger = None
    callbacks = []
    #host = "127.0.0.1"
    host="82.156.36.121"
    serverId=0
    def init(self, requester, configReader, logger):
        self.requester = requester
        self.configReader = configReader
        self.logger = logger
	self.serverId= configReader.getCommonConfig().get("serverId")
	#data={"playerGUID":"76561198324874244","playerName":"GangGang"}
	#self.clientSynthAdd(None,data)
    def clientSynthAdd(self, log, data):
        print("JoinMsgPlugin clientSynthAdd 接收" + str(data))
        playerGUID = data.get("playerGUID")
        playerName = data.get("playerName")
        if playerGUID is None:
            return
	url='http://' + self.host + '/insurgency_web/player_join_msg.php?playerGUID=' + playerGUID + '&playerName=' + playerName+'&serverId='+str(self.serverId)
        r = requests.get(url)
        dataStr = str(r.text)
       	print("请求:"+url+"\ndata:"+dataStr)
	data = json.loads(dataStr)
        if data["code"] == 0 and len(data["data"]) > 0:
	    msg=str(data["data"])
            self.requester.apiSay(str(data["data"]))
        else:
            print(dataStr)

    def roundStateChange(self, log, data):
        print("JoinMsgPlugin roundStateChange 接收" + str(data))
