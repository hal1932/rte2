# encoding: utf-8
import sys
import os
sys.path.append(os.path.join(os.getcwd(), "..", "SwigPy"))

import unittest
import rte2 as rt

import math
import gc


class Test(unittest.TestCase):
    def setUp(self):
        gc.enable()
        #self.root = rt.Node.createRootNode("root", "lroot")
        self.root = rt.createRootNode("root", "lroot")

    def tearDown(self):
        #rt.Node.destroy(self.root)
        rt.destroyRootNode(self.root)
        gc.collect()
        gc.set_debug(gc.DEBUG_LEAK)
        sys.stderr.flush()

    def test_node(self):
        node = self.root.addChild("name", "label")
        self.assertEqual(node.getName(), "name")
        self.assertEqual(node.getLabel(), "label")
        self.assertEqual(node.getPath(), "root/name")

        node.setName("name1")
        self.assertEqual(node.getName(), "name1")
        self.assertEqual(node.getPath(), "root/name1")

        node.setLabel("label1")
        self.assertEqual(node.getLabel(), "label1")

        child = node.addChild("child", "childLabel")
        self.assertEqual(child.getParent(), node)
        self.assertEqual(child.getPath(), "root/name1/child")
        self.assertEqual(node.getChildren().size(), 1)
        self.assertEqual(node.getChildren()[0], child)
        self.assertEqual(node.findChild("child"), child)
        self.assertEqual(node.findChild(child), child)

        result = node.removeChild(child)
        self.assertEqual(result, True)
        self.assertEqual(node.getChildren().size(), 0)
        self.assertEqual(node.findChild("child"), None)
        self.assertEqual(node.findChild(child), None)

    def test_nodeContent(self):
        node = self.root.addChild("name", "label")

        c = node.createContent()
        self.assertEqual(c, node.getContent())
        self.assertEqual(c.getPath(), "root/name/" + c.getName())

        c.setName("cname")
        self.assertEqual(c.getName(), "cname")
        self.assertEqual(c.getPath(), "root/name/cname")

        c.setLabel("clabel")
        self.assertEqual(c.getLabel(), "clabel")

    def test_nodeContentData(self):
        c = self.root.createContent()

        i = c.createDataInt32()
        self.assertEqual(i, c.getDataInt32())

        i.Value = 1
        self.assertEqual(c.getDataInt32(), i)
        self.assertEqual(c.getDataInt32().Value, i.Value)
        self.assertEqual(i.getType(), rt.NodeContentData.Type_Int32)
        self.assertEqual(i.getType(), c.getDataType())

    def test_serialize(self):
        node = self.root.addChild("node", "lnode")

        c = node.createContent()
        c.setName("cn")
        c.setLabel("cl")

        d = c.createDataInt32()
        d.Value = 1

        size = self.root.calcSize()
        buf = rt.buffer(size)
        self.root.serialize(buf.cast())

        root = rt.Node()
        root.deserialize(buf.cast())
        self.assertEqual(root.getName(), self.root.getName())
        self.assertEqual(root.getPath(), self.root.getPath())
        self.assertEqual(root.getLabel(), self.root.getLabel())
        self.assertEqual(root.getChildren().size(), 1)

        n = root.getChildren()[0]
        self.assertEqual(n.getParent(), root)
        self.assertEqual(n.getName(), node.getName())
        self.assertEqual(n.getPath(), node.getPath())
        self.assertEqual(n.getLabel(), node.getLabel())

        c1 = n.getContent()
        self.assertEqual(c1.getName(), c.getName())
        self.assertEqual(c1.getPath(), c.getPath())
        self.assertEqual(c1.getLabel(), c.getLabel())
        self.assertEqual(c1.getDataType(), c.getDataType())

        d1 = c1.getDataInt32()
        self.assertEqual(d1.getType(), d.getType())
        self.assertEqual(d1.Value, d.Value)

if __name__ == "__main__":
    unittest.main()
