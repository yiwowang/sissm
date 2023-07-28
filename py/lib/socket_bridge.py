# !/usr/bin/env python
# -*- coding: utf-8 -*

import random
import socket
import json
import threading
import time
import traceback
from concurrent.futures import ThreadPoolExecutor
from lib.event_dispatcher import *

# import sys
# reload(sys)
# sys.setdefaultencoding('utf8')


HOST = '127.0.0.1'
PORT = 8006
ADDR = (HOST, PORT)
ENCODING = 'utf-8'
BUFFSIZE = 1024
MAX_LISTEN = 5
socketObj = None
debug=False
retryDelayArray=[1,3,6,10,20,40,60,2*60,4*60,8*60,16*60,32*60]
class SocketWorker:
    callbackList = []
    resultMap = {}
    socketObj = None
    threadPool = ThreadPoolExecutor(max_workers=2)
    loopStart = True

    def send(self, data):
        print("发送：" + str(data))
        result = None
        if self.socketObj is not None:
            condition = threading.Condition()
            if condition.acquire():
                key = str(time.time()) + str(random.randint(10000, 100000))
                data["requestId"] = key
                self.resultMap[key] = {"condition": condition, "result": ""}
                self.socketObj.send((json.dumps(data)).encode("UTF-8"))
                condition.wait(timeout=10)
                result = self.resultMap.pop(key)["result"]
            condition.release()
        return result

    def dispatchEvent(self, jsonObject):
        try:
            eventDispatcher.dispatch(jsonObject)
        except Exception as e:
            traceback.print_exc()

    def sendTest(self):
        ret = self.send({})
        print("收到返回:" + str(ret))

    def socketLoop(self):
        retryCount = 0
        while self.loopStart:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # 绑定服务器地址和端口
            try:
                s.connect((HOST, PORT))
                print('等待用户接入。。。。。。。。。。。。')
                self.socketObj = s
                retryCount = 0
                print('连接成功')
                while self.loopStart:
                    # 接收返回数据
                    outData = s.recv(BUFFSIZE)
                    try:
                        msgs = outData.decode("UTF-8")
			if msgs is None or len(msgs)==0:
			    s.close()
		            break
                        if "\n" not in msgs:
			    time.sleep(1)                            
			    continue
			    
                        arr = msgs.split("\n")
                        for i in range(0, len(arr)):
                            msg = arr[i]
                            if len(msg) == 0:
                                continue
		            if debug:
                            	print('返回数据信息:'+msg)
                            try:
                                jsonObject = json.loads(msg, strict=False)
                            except Exception as e:
                                print("^json 解析错误:消息不完整：" + msg)
                                continue
                            if "requestId" in jsonObject:
                                # 客户端请求的响应结果
                                sendKey = jsonObject["requestId"]
                                value = self.resultMap.get(sendKey)
                                if value is None:
                                    continue
                                condition = value.get("condition")
                                if condition is None:
                                    continue
                                if condition.acquire():
                                    value["result"] = jsonObject
                                    condition.notify()
                                condition.release()
                            else:
                                # 服务端主动发送的事件
                                self.threadPool.submit(self.dispatchEvent, jsonObject)
                    except Exception as e:
                        traceback.print_exc()
                        continue
            except Exception as e:
                traceback.print_exc()
		retryCount += 1
                print("正在重试第" + str(retryCount) + "次")
                s.close()
		if retryCount<10:
		    time.sleep(5)
		else:
		    time.sleep(60)
		
