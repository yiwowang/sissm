#sissm_python说明#

基于sissm开发的python版插件，原理是用python和sissm建立通信，
这样用户就可以使用python写代码了。

一.为什么做做sissm_python？
因为sissm是C语言编写的，开发成本特别大，而python开发成本低效

二.如何使用？
1、首先更新我们的sissm，
下载地址：
2、 然后开启插件：
socket_plugin.pluginState 1

(1)如果你会python
    那么你可以继承EventCallback，并且在Register注册插件。
    使用requester对象可以获取你想要的数据，以及执行rcon命令。
    具体可以参考NoCodePlugin

(2)如你不会python
    那么你只能使用NoCodePlugin插件实现功能了。
    接下来介绍NoCodePlugin使用方法。
    你只需要修改文件config/no_code_config.json即可。
文件格式如下：
{
  "events": [
    {
      "事件名1": "条件1",
      "事件名2": "条件2",
      。。。。
      "cmds": [
        "rcon命令1"   ,
        "rcon命令2"
      ]
    }
  ]
}
events是一个集合，每个元素都是由条件+cmd组成，简单理解就是，当事件发生时，且满足条件，就执行对应的cmd。
条件支持python表达式

举个例子例如
{
  "events": [
    {
      "anyEvent": "log.find(\"kill\")>0",
      "roundStart": "playerCount>-2",
      "cmds": [
        "say hello",
        "gamemodeproperty MinimumEnemies 4",
        "gamemodeproperty MaximumEnemies 40"
      ]
    }
  ]
}

支持的事件：
anyEvent
clientAdd
clientDel
restart
mapChange
gameStart
gameEnd
roundStart
roundEnd
roundStateChange
takeObject
killed
captured
shutdown
clientSynthDel
clientSynthAdd
chat
sigterm
winLose
travel
sessionLog
objectSynth
everyLog

条件里支持的全局变量
playerCount
AIDeadCountForGame
AIDeadCountForCurrRound
AIDeadCountForCurrPoint
PlayerDeadCountForGame
PlayerDeadCountForCurrRound
PlayerDeadCountForCurrPoint
roundIndex



通用配置：
config/common_config.json
enableLog用来开启或关闭日志

