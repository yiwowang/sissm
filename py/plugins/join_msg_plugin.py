# !/usr/bin/env python
# -*- coding: utf-8 -*
import json
import sys
reload(sys)
sys.setdefaultencoding('utf8')
import requests as requests
from lib.event_callback import EventCallback
from lib.jianti_fanti import Translate
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
	self.translate = Translate()
	#print(self.translate.ToTraditionalChinese("测试账号你好中国"))	
	#ppp="测试账号"	
	#data={"playerGUID":"76561198324874244","playerName":ppp}
	#self.clientSynthAdd(None,data)
    def clientSynthAdd(self, log, data):
        print("JoinMsgPlugin clientSynthAdd 接收" + str(data))
        playerGUID = data.get("playerGUID")
        playerName = data.get("playerName").encode('utf-8')
	print(self.translate.ToTraditionalChinese(playerName))
	playerNameFanTi = self.translate.ToTraditionalChinese(playerName)
	if playerGUID is None:
            return
	url='http://' + self.host + '/insurgency_web/player_join_msg.php?playerGUID=' + playerGUID + '&playerName=' + playerName+'&playerNameFanTi='+playerNameFanTi+'&serverId='+str(self.serverId)
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
