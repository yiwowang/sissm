 #!/usr/bin/env python
 # -*- coding: utf-8 -*


class Requester:
    socketWorker = None

    def __init__(self, socketWorker):
        self.socketWorker = socketWorker

    def apiGameModePropertySet(self, name, value):
       return self.requestApi("apiGameModePropertySet", {name: value})

    def requestApi(self, requestName, requestParam):
        data = {}
        data["requestName"] = requestName
        data["requestType"] = "request"
        if requestParam is not None:
            for key in requestParam.keys():
                data["requestParams"] = key + "|" + requestParam[key]
        return self.socketWorker.send(data)

    def requestRconCmd(self, rconCmd):
        data = {}
        data["requestName"] = "apiRcon"
        data["requestType"] = "request"
        data["requestParams"] = rconCmd
        return self.socketWorker.send(data)
