#!/usr/bin/env python
# -*- coding: utf-8 -*
import json

from event import *
from requester import Requester
from socket_bridge import SocketWorker

socketWorker = None
requester = None


class VoteMapPlugin(EventCallback):
    def clientAdd(self, log, player):
        print("VoteMapPlugin 接收" + str(log))
        # cmd = {"log": "123"}
        # print("VoteMapPlugin 发送命令:" + str(cmd))
        # result = socketWorker.send(cmd)
        # print("VoteMapPlugin 命令结果:" + json.dumps(result))


class ChatPlugin(EventCallback):
    def clientAdd(self, log, player):
        print("ChatPlugin " + log)
        ret1 = requester.apiGameModePropertySet("game_count", "1")
        print("apiGameModePropertySet ret=" + str(ret1))
        ret2 = requester.requestRconCmd("game_xxxxx")
        print("requestRconCmd ret=" + str(ret2))



if __name__ == "__main__":
    print("hello,world!")

    socketWorker = SocketWorker()
    requester = Requester(socketWorker)

    plugin = VoteMapPlugin()
    plugin2 = ChatPlugin()
    eventDispatcher.register(plugin)
    eventDispatcher.register(plugin2)
    eventDispatcher.init(requester)

    socketWorker.socketLoop()
