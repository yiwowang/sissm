#!/usr/bin/env python
# -*- coding: utf-8 -*
from lib.config_reader import ConfigReader
from plugins.join_msg_plugin import JoinMsgPlugin
from plugins.no_code_plugin import NoCodePlugin
from lib.event_dispatcher import *
from lib.requester import Requester
from lib.socket_bridge import SocketWorker
from register import Register

socketWorker = None

if __name__ == "__main__":
    socketWorker = SocketWorker()
    requester = Requester(socketWorker)
    register = Register()
    pluginList = []
    register.getPluginList(pluginList)
    for plugin in pluginList:
        eventDispatcher.register(plugin)
    configReader = ConfigReader()
    logger = Logger(configReader.getCommonConfig().get("enableLog"))
    eventDispatcher.init(requester, configReader, logger)
    # data = {"log": "", "playerGUID": "76561198324874244", "playerName": "血战钢锯岭", "event_type": "clientSynthAdd"}
    # eventDispatcher.dispatch(data)

    # log = "[聊天消息] [2022.06.16-14.06.39:160][754]LogChat: Display: K2F5(76561198325604853) Global Chat: 我们就来坑你了"
    # data = {"log": log, "playerGUID": "76561198324874244", "playerName": "血战钢锯岭", "event_type": "chat"}
    # eventDispatcher.dispatch(data)
    socketWorker.socketLoop()
