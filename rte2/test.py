# encoding: utf-8
import sys
import os
sys.path.append(os.path.join(os.getcwd(), "..", "SwigPy"))

import unittest
import rte2 as rt

import math


class Test(unittest.TestCase):
    def setUp(self):
        self.context = rt.Context()
        self.root = self.context.addNode("root", None)
        self.param0 = rt.NodeParameter("param0", self.root, rt.ParameterType_Int32)

    def tearDown(self):
        self.param0 = None
        self.root = None
        self.context = None

    def test_node(self):
        self.assertEqual(self.root.getName(), "root")
        self.assertEqual(self.root.getPath(), "/root")

    def test_param(self):
        self.assertEqual(self.param0.getName(), "param0")
        self.assertEqual(self.param0.getPath(), "/root/param0")
        self.assertEqual(self.param0.getType(), rt.ParameterType_Int32)
        self.assertEqual(self.param0.getParent().getName(), "root")
        self.assertEqual(self.param0.getParent().getPath(), "/root")
        self.assertEqual(len(self.root.getChildren()), 0)
        self.assertEqual(len(self.root.getParameterList()), 1)
        self.assertEqual(self.root.getParameterList()[0].getName(), "param0")

        self.param0.setValueInt32(1)
        self.assertEqual(self.param0.getValueInt32(), 1)

    def test_node_serialize(self):
        buf = rt.buffer(256)
        self.root.serialize(buf.cast(), self.context)

        #print "---"
        #import struct
        #s = struct.pack("256B", *[buf[x] for x in xrange(256)])
        #print len(s)
        #for a in s: print a
        #print "---"

        node = rt.Node()
        node.deserialize(buf.cast(), self.context)

        self.assertEqual(node.getName(), "root")
        self.assertEqual(node.getPath(), "/root")

        params = node.getParameterList()
        self.assertEqual(len(params), 1)
        self.assertEqual(params[0].getName(), "param0")
        self.assertEqual(params[0].getPath(), "/root/param0")
        self.assertEqual(params[0].getType(), rt.ParameterType_Int32)

    def test_param_serialize(self):
        buf = rt.buffer(1024)
        self.param0.serialize(buf.cast(), self.context)

        param = rt.NodeParameter()
        param.deserialize(buf.cast(), self.context)

        self.assertEqual(param.getName(), "param0")
        self.assertEqual(param.getPath(), "/root/param0")
        self.assertEqual(param.getType(), rt.ParameterType_Int32)
        self.assertEqual(param.getParent().getName(), "root")
        self.assertEqual(param.getParent().getPath(), "/root")

        rt.NodeParameter.setFileParameterRoot(os.path.join("D:/", "tmp"))
        param1 = rt.NodeParameter("param1", self.root, rt.ParameterType_File)
        f = rt.File()
        f.path = os.path.join("D:", "tmp", "test.py")
        param1.setValueFile(f)
        param1.serialize(buf.cast(), self.context)

        rt.NodeParameter.setFileParameterRoot(os.path.join("D:/", "tmp", "test"))
        param2 = rt.NodeParameter()
        param2.deserialize(buf.cast(), self.context)
        self.assertEqual(param2.getName(), "param1")
        self.assertEqual(param2.getPath(), "/root/param1")
        self.assertEqual(param2.getType(), rt.ParameterType_File)
        f2 = param2.getValueFile()
        self.assertEqual(f2.path, "D:/tmp/test/test.py")

    def test_ping_command(self):
        cmd = rt.PingCommand()
        self.assertTrue(cmd.getType() == rt.CommandType_Ping)

        buf = rt.buffer(256)
        cmd.serialize(buf.cast(), self.context)

        cmd1 = rt.PingCommand()
        cmd1.deserialize(buf.cast(), self.context)
        self.assertTrue(cmd1.getType() == rt.CommandType_Ping)

    def test_NodeNameList_command(self):
        cmd = rt.NodeNameListCommand()
        self.assertEqual(cmd.getType(), rt.CommandType_NodeNameList)
        
        cmd.add(self.root)
        self.assertEqual(",".join(cmd.getNodePathList()), "/root")

        child = self.context.addNode("child", self.root)
        cmd.add(child)
        self.assertEqual(",".join(cmd.getNodePathList()), "/root,/root/child")

        buf = rt.buffer(512)
        cmd.serialize(buf.cast(), self.context)

        cmd1 = rt.NodeNameListCommand()
        cmd1.deserialize(buf.cast(), self.context)
        self.assertEqual(cmd1.getType(), rt.CommandType_NodeNameList)
        nodeList = cmd.getNodePathList()
        self.assertEqual(",".join(nodeList), "/root,/root/child")
        tree = rt.NodeNameListCommand.createNodeTree(nodeList)
        self.assertEqual(len(tree), 2)
        self.assertEqual(tree[0].getPath(), "/root")
        self.assertEqual(tree[1].getPath(), "/root/child")
        self.assertEqual(tree[0].getChildren()[0].getPath(), tree[1].getPath())

    def test_Node_command(self):
        cmd = rt.NodeCommand()
        self.assertEqual(cmd.getType(), rt.CommandType_Node)

        cmd.set(self.root)

        buf = rt.buffer(256)
        cmd.serialize(buf.cast(), self.context)

        cmd1 = rt.NodeCommand()
        cmd1.deserialize(buf.cast(), self.context)
        self.assertEqual(cmd1.getType(), rt.CommandType_Node)
        self.assertEqual(cmd1.get().getName(), "root")
        self.assertEqual(cmd1.get().getPath(), "/root")
        self.assertEqual(cmd1.get().getParent(), None)

    def test_paramUpdate_command(self):
        cmd = rt.ParamUpdateCommand()
        self.assertEqual(cmd.getType(), rt.CommandType_ParamUpdate)

        self.param0.setValueInt32(2)
        cmd.add(self.param0)
        self.assertEqual(len(cmd.getParameterList()), 1)
        self.assertEqual(cmd.getParameterList()[0].getName(), "param0")
        self.assertEqual(cmd.getParameterList()[0].getPath(), "/root/param0")
        self.assertEqual(cmd.getParameterList()[0].getValueInt32(), 2)

        param1 = rt.NodeParameter("param1", self.root, rt.ParameterType_Float32)
        param1.setValueFloat32(3.0)
        cmd.add(param1)

        param2 = rt.NodeParameter("param2", self.root, rt.ParameterType_Vector3)
        v = rt.Vector3()
        v.x = 1.0
        v.y = 2.0
        v.z = 3.0
        param2.setValueVector3(v)
        cmd.add(param2)

        param3 = rt.NodeParameter("param3", self.root, rt.ParameterType_String)
        param3.setValueString("aaaaa")
        cmd.add(param3)

        buf = rt.buffer(256)
        cmd.serialize(buf.cast(), self.context)

        cmd1 = rt.ParamUpdateCommand()
        cmd1.deserialize(buf.cast(), self.context)
        self.assertEqual(cmd1.getType(), rt.CommandType_ParamUpdate)

        params = cmd1.getParameterList()
        self.assertEqual(len(params), 4)

        for p in params:
            parent = p.getParent()
            self.assertIsNotNone(parent)
            self.assertEqual(parent.getName(), "root")
            self.assertEqual(parent.getPath(), "/root")

        p = params[0]
        self.assertEqual(p.getName(), "param0")
        self.assertEqual(p.getPath(), "/root/param0")
        self.assertEqual(p.getType(), rt.ParameterType_Int32)
        self.assertEqual(p.getValueInt32(), 2)

        p = params[1]
        self.assertEqual(p.getName(), "param1")
        self.assertEqual(p.getPath(), "/root/param1")
        self.assertEqual(p.getType(), rt.ParameterType_Float32)
        self.assertEqual(math.ceil(p.getValueFloat32()), 3)

        p = params[2]
        self.assertEqual(p.getName(), "param2")
        self.assertEqual(p.getPath(), "/root/param2")
        self.assertEqual(p.getType(), rt.ParameterType_Vector3)
        v = p.getValueVector3()
        self.assertEqual(math.ceil(v.x), 1)
        self.assertEqual(math.ceil(v.y), 2)
        self.assertEqual(math.ceil(v.z), 3)

        p = params[3]
        self.assertEqual(p.getName(), "param3")
        self.assertEqual(p.getPath(), "/root/param3")
        self.assertEqual(p.getType(), rt.ParameterType_String)
        self.assertEqual(p.getValueString(), "aaaaa")

    def test_socket(self):
        rt.Socket.setup()
        rt.Socket.shutdown()

if __name__ == "__main__":
    unittest.main()
