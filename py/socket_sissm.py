 #!/usr/bin/env python
 # -*- coding: utf-8 -*

import socket
import time

HOST = ''
PORT = 8000
ADDR = (HOST, PORT)
BUFFSIZE = 1024
MAX_LISTEN = 5


def tcpServer():
    # TCP服务
    # with socket.socket() as s:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # 绑定服务器地址和端口
        s.bind(ADDR)
        # 启动服务监听
        s.listen(MAX_LISTEN)
        print('等待用户接入。。。。。。。。。。。。')
        while True:
            # 等待客户端连接请求,获取connSock
            conn, addr = s.accept()
            print('警告，远端客户:{} 接入系统！！！'.format(addr))
            with conn:
                conn.setblocking(True)
                while True:
                    print('接收请求信息。。。。。')
                    # 发送请求数据
                    conn.send(
                        "{\"event_type\":\"ClientAdd\",\"log\":\"[2022.07.26-15.33.39:321][748]LogNet: Join succeeded: 血战钢锯岭\"}\n".encode(
                            "UTF-8"))
                    print('发送返回完毕！！！')
                    # 接收请求信息
                    data = conn.recv(BUFFSIZE)
                    print('data=%s' % data)
                    print('接收数据：{!r}'.format(data.decode('utf-8')))
                    #todo 发送前每条消息末尾加回车
                    conn.send((data.decode('utf-8') + "\n").encode("UTF-8"))
        s.close()


if __name__ == '__main__':
    tcpServer()
