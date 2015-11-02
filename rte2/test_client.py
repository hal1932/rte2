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
    print "sendAsync", client.sendAsync(b"1234", 4)

    while True:
        received = client.getReceivedQueue()
        for data in received:
            if data.bufferSize > 0:
                print data.buffer, data.bufferSize
            else:
                print "error"
        
        if not client.isConnectionAlive():
            print "closed"
            break
        
        time.sleep(0.1)

    client.close()
    rt.Socket.shutdown()

if __name__ == "__main__":
    main()
