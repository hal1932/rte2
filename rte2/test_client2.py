# encoding: utf-8
import sys
import os
sys.path.append(os.path.join(os.getcwd(), "..", "SwigPy"))

import rte2 as rt
import time

def main():
    rt.core.setup()

    rootNode = rt.Node.createRootNode("root", "root_label")

    child1 = rootNode.addChild("child1", "child1_label")
    content1 = child1.createContent("content1", "content1_label")
    data1 = content1.createDataInt32()
    data1.Value = 1234

    bufSize = rootNode.calcSize()
    buf = rt.buffer(bufSize)
    rootNode.serialize(buf.cast())

    client = rt.TcpClient()
    client.connect("127.0.0.1", 0x1234)
    client.sendAsync(buf.cast(), bufSize)

    # send
    while True:
        if len(client.popSentQueue()) > 0:
            break
        time.sleep(10)

    # receive, deserialize
    while True:
        if not client.isConnectionAlive():
            break

        received = client.popReceivedQueue()
        if len(received) > 0:
            data = received[0]

            ptr = data.buffer
            while ptr.cast() != data.buffer.cast() + data.bufferSize:
                n = rt.Node()
                ptr = n.deserialize(ptr)
                print n.getName()
                rt.Node.destroy(n)
            data.deallocate()

            print "success!"
            break

        time.sleep(10)

    rt.Node.destroy(rootNode)
    
    client.close()
    rt.core.shutdown()

if __name__ == "__main__":
    main()
