# import queue
# import random
# import socket
# import json
# import threading
# import time
# from time import ctime
# import traceback
# from concurrent.futures import ThreadPoolExecutor
# from event import *
#
# HOST = '127.0.0.1'
# PORT = 8000
# ADDR = (HOST, PORT)
# ENCODING = 'utf-8'
# BUFFSIZE = 1024
#
# socketObj = None
#
#
# class SocketWorker:
#     callbackList = []
#     resultMap = {}
#     socketObj = None
#     threadPool = ThreadPoolExecutor(max_workers=2)
#     loopStart = True
#
#     def send(self, map):
#         result = None
#         if self.socketObj is not None:
#             condition = threading.Condition()
#             if condition.acquire():
#                 key = str(time.time()) + str(random.randint(10000, 100000))
#                 map["send_key"] = key
#                 self.resultMap[key] = {"condition": condition, "result": ""}
#                 self.socketObj.send((json.dumps(map)).encode("UTF-8"))
#                 condition.wait(timeout=5)
#                 result = self.resultMap.pop(key)["result"]
#             condition.release()
#         return result
#
#     def dispatchEvent(self, jsonObject):
#         try:
#             eventDispatcher.dispatch(jsonObject["log"], jsonObject)
#         except Exception as e:
#             print("Error: " + str(e))
#             traceback.print_exc()
#
#     def socketLoop(self):
#         # 创建客户套接字
#         with socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM) as s:
#             self.socketObj = s
#             s.setblocking(True)
#             # 尝试连接服务器
#             s.connect(ADDR)
#             print('连接服务成功！！')
#             # 通信循环
#             while self.loopStart:
#                 # inData = input('pleace input something:')
#                 # if inData == 'q':
#                 #     break
#                 # # 发送数据到服务器
#                 # inData = '[{}]:{}'.format(ctime(), inData)
#                 #
#                 # s.send(inData.encode(ENCODING))
#                 # print('发送成功！')
#
#                 # 接收返回数据
#                 outData = s.recv(BUFFSIZE)
#                 msg = outData.decode("UTF-8")
#                 if "\n" in msg:
#                     arr = msg.split("\n")
#                     for msg in arr:
#                         if len(msg) == 0:
#                             continue
#                         print('返回数据信息：{!r}'.format(msg))
#                         jsonObject = json.loads(msg)
#                         if "send_key" in jsonObject:
#                             sendKey = jsonObject["send_key"]
#                             value = self.resultMap[sendKey]
#                             condition = value["condition"]
#                             if condition.acquire():
#                                 value["result"] = jsonObject
#                                 condition.notify()
#                             condition.release()
#                         else:
#                             self.threadPool.submit(self.dispatchEvent, jsonObject)
#
#             # 关闭客户端套接字
#             s.close()
