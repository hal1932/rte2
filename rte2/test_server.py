# encoding: utf-8
import sys
import os
sys.path.append(os.path.join(os.getcwd(), "..", "SwigPy"))

import time
import rte2 as rt

def main():
    rt.Socket.setup()

    server = rt.TcpServer()
    print "open", server.open(0x1234)

    while True:
        acceped = server.getAcceptedQueue()
        for clientId in acceped:
            print "accept: " + clientId
            
            clientCount = server.getClientCount()
            if clientCount > 0:
                clients = server.getClientList()
                for client in clients:
                    print "client: ", client.Id
            
            server.sendAsync(clientId, b"accpet", 6)
        
        received = server.popReceivedQueue()
        for data in received:
            if data.bufferSize > 0:
                print data.clientId, data.buffer, data.bufferSize
                server.sendAsync(data.clientId, data.buffer, data.bufferSize) # echo back
            else:
                print "error recv: ", data.clientId
                server.closeConnection(data.clientId)

        send = server.popSentQueue()
        for data in send:
            if data.bufferSize == data.sendSize:
                print "send: ", data.clientId
            else:
                print "error send: ", data.clientId
                server.closeConnection(data.clientId)
        
        closed = server.popClosedQueue()
        for clientId in cloesd:
            print "closed: ", clientId
        
        time.sleep(0.1)

    server.close()
    rt.Socket.shutdown()

if __name__ == "__main__":
    main()
