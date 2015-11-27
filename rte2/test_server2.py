# encoding: utf-8
import sys
import os
sys.path.append(os.path.join(os.getcwd(), "..", "SwigPy"))

import rte2 as rt
import time

def main():
    rt.core.setup()
    
    server = rt.TcpServer()
    server.open(0x1234)

    while True:
        clients = server.popAcceptedQueue()
        for c in clients:
            print "accept ", c

        received = server.popReceivedQueue()
        for r in received:
            if r.bufferSize > 0:
                print "received from: ", r.clientId
                # echo back
                server.sendAsync(r.clientId, r.buffer, r.bufferSize)

        sents = server.popSentQueue()
        for s in sents:
            print "sent to ", s.clientId

        closed = server.popClosedQueue()
        for c in closed:
            print "closed ", c

        time.sleep(0.1)

    rt.core.shutdown()

if __name__ == "__main__":
    main()
