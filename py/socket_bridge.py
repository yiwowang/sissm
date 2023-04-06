# !/usr/bin/env python
# -*- coding: utf-8 -*

import random
import socket
import json
import threading
import time
from time import ctime
import traceback
from concurrent.futures import ThreadPoolExecutor
from event import *

HOST = '127.0.0.1'
PORT = 8000
ADDR = (HOST, PORT)
ENCODING = 'utf-8'
BUFFSIZE = 1024
MAX_LISTEN = 5
socketObj = None


class SocketWorker:
    callbackList = []
    resultMap = {}
    socketObj = None
    threadPool = ThreadPoolExecutor(max_workers=2)
    loopStart = True

    def send(self, data):
        result = None
        if self.socketObj is not None:
            condition = threading.Condition()
            if condition.acquire():
                key = str(time.time()) + str(random.randint(10000, 100000))
                data["requestId"] = key
                self.resultMap[key] = {"condition": condition, "result": ""}
                self.socketObj.send((json.dumps(data)).encode("UTF-8"))
                print("发送：" + str(data))
                condition.wait(timeout=10)
                result = self.resultMap.pop(key)["result"]
            condition.release()
        return result

    def dispatchEvent(self, jsonObject):
        try:
            print("dispatchEvent")
            eventDispatcher.dispatch(jsonObject["log"], jsonObject)
        except Exception as e:
            print("Error: " + str(e))
            traceback.print_exc()

    def sendTest(self):
        ret = self.send({})
        print("收到返回:" + str(ret))

    def socketLoop(self):
        while self.loopStart:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # 绑定服务器地址和端口
            s.bind(ADDR)
            # 启动服务监听
            s.listen(MAX_LISTEN)
            print('等待用户接入。。。。。。。。。。。。')
            # 等待客户端连接请求,获取connSock
            conn, addr = s.accept()
            self.socketObj = conn

            print('警告，远端客户:{} 接入系统！！！'.format(addr))
            conn.setblocking(True)
            while self.loopStart:
                # 接收返回数据
                outData = conn.recv(BUFFSIZE)
                msg = outData.decode("UTF-8")
                if "\n" in msg:
                    arr = msg.split("\n")
                    for msg in arr:
                        if len(msg) == 0:
                            continue
                        print('返回数据信息：{!r}'.format(msg))
                        try:
                            jsonObject = json.loads(msg, strict=False)
                        except Exception as e:
                            print("e:" + str(e))
                            continue
                        if "requestId" in jsonObject:
                            sendKey = jsonObject["requestId"]
                            value = self.resultMap[sendKey]
                            condition = value["condition"]
                            if condition.acquire():
                                value["result"] = jsonObject
                                condition.notify()
                            condition.release()
                        else:
                            self.threadPool.submit(self.dispatchEvent, jsonObject)
