# encoding: utf-8
import sys
import os
sys.path.append(os.path.join(os.getcwd(), "..", "SwigPy"))

import unittest
import rte2 as rt

import math


class Test(unittest.TestCase):
    def setUp(self):
        self.root = rt.Node("root", "rlabel", None)

    def tearDown(self):
        self.root = None

    def test_node(self):
        node = rt.Node("name", "label", self.root)
        self.assertEqual(node.getName(), "name")
        self.assertEqual(node.getLabel(), "label")
        self.assertEqual(node.getPath(), "root/name")

        node.setName("name1")
        self.assertEqual(node.getName(), "name1")
        self.assertEqual(node.getPath(), "root/name1")

        node.setLabel("label1")
        self.assertEqual(node.getLabel(), "label1")

        child = rt.Node("child", "childLabel", node)
        node.addChild(child)
        self.assertEqual(child.getParent(), node)
        self.assertEqual(child.getPath(), "root/name1/child")
        self.assertEqual(node.getChildren().size(), 1)
        self.assertEqual(node.getChildren()[0], child)
        self.assertEqual(node.findChild("child"), child)
        self.assertEqual(node.findChild(child), child)

        removed = node.removeChild(child)
        self.assertEqual(removed, child)
        self.assertEqual(node.getChildren().size(), 0)
        self.assertEqual(node.findChild("child"), None)
        self.assertEqual(node.findChild(child), None)

        node.addChild(removed)
        removed = node.removeChild(child)
        self.assertEqual(removed, child)

    def test_nodeContent(self):
        node = rt.Node("name", "label", self.root)
        node.createContent()

        c = node.getContent()
        self.assertEqual(c.getPath(), "root/name/" + c.getName())

        c.setName("cname")
        self.assertEqual(c.getName(), "cname")
        self.assertEqual(c.getPath(), "root/name/cname")

        c.setLabel("clabel")
        self.assertEqual(c.getLabel(), "clabel")


if __name__ == "__main__":
    unittest.main()
