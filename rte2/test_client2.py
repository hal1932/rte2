# encoding: utf-8
import sys
import os
sys.path.append(os.path.join(os.getcwd(), "..", "SwigPy"))

import rte2 as rt
import time
import gc

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
        time.sleep(0.1)

    # receive, deserialize
    while True:
        if not client.isConnectionAlive():
            print "connection is closed by server"
            break

        received = client.popReceivedQueue()
        if len(received) > 0:
            print "received"
            data = received[0]

            context = rt.NodeDeserializationContext(data.buffer, data.bufferSize)
            while context.hasNext():
                n = context.getNext()
                print n.getName()
            
            data.deallocate()

            print "success!"
            break

        time.sleep(0.1)
    
    client.close()
    rt.core.shutdown()

if __name__ == "__main__":
    gc.enable()
    main()
    gc.collect()
    gc.set_debug(gc.DEBUG_LEAK)
