# encoding: utf-8
import sys
import os
sys.path.append(os.path.join(os.getcwd(), "..", "SwigPy"))

import time
import rte2 as rt

def main():
    rt.Socket.setup()
    
    client = rt.TcpClient()
    print "connect", client.connect("127.0.0.1", 0x1234)
    print "sendAsync", client.sendAsync(rt.makeArray("1234"), 4)

    closed = False

    while True:
        send = client.popSentQueue()
        for data in send:
            if data.bufferSize == data.sentSize:
                print "send"
            else:
                print "send error"
                closed = True

        received = client.popReceivedQueue()
        for data in received:
            closed = True
            if data.bufferSize > 0:
                print "received", data.buffer, data.bufferSize
            else:
                print "receive error"

        if closed:
            break
        
        time.sleep(0.1)

    client.close()
    print "close"

    rt.Socket.shutdown()
    print "shutdown"

if __name__ == "__main__":
    main()
