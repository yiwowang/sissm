from plugins.join_msg_plugin import JoinMsgPlugin
from plugins.no_code_plugin import NoCodePlugin
from plugins.test_plugin import TestPlugin


class Register:
    def getPluginList(self, pluginList):
        pluginList.append(TestPlugin())
        pluginList.append(NoCodePlugin())
        pluginList.append(JoinMsgPlugin())
