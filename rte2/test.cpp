#ifndef _SWIG_PY

#include "Node.h"
#include "NodeParameter.h"
#include "Context.h"
#include "Command.h"
#include "Socket.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <map>
#include <iostream>
#include <memory>

int _main(int, char**)
{
	rte::Context context;

	rte::Node* root = context.addNode("root", nullptr);
	rte::Node* child = context.addNode("child", root);

	std::unique_ptr<uint8_t> buffer(new uint8_t[1024]);

	rte::NodeParameter param1("param1", root, rte::ParameterType::Int32);
	param1.setValue(1);

	rte::NodeParameter param2("param2", root, rte::ParameterType::Int32);
	param2.setValue(2);

	rte::NodeParameter param3("param3", root, rte::ParameterType::String);
	param3.setValue(std::string("abcde"));

	{
		rte::PingCommand pc, pc1;
		pc.serialize(buffer.get(), &context);
		pc1.deserialize(buffer.get(), &context);
	}
	{
		rte::NodeNameListCommand qnnc, qnnc1;
		qnnc.add(root);
		qnnc.add(child);
		qnnc.serialize(buffer.get(), &context);
		qnnc1.deserialize(buffer.get(), &context);
	}
	{
		rte::NodeCommand qnc, qnc1;
		qnc.set(root);
		qnc.serialize(buffer.get(), &context);
		qnc1.deserialize(buffer.get(), &context);
	}
	{
		rte::NodeNameListCommand c, c1;
		c.add(root);
		c.add(child);
		c.serialize(buffer.get(), &context);
		c1.deserialize(buffer.get(), &context);
		const std::vector<std::string>& pathList = c1.getNodePathList();
		assert(pathList.size() == 2);

		auto tree = rte::NodeNameListCommand::createNodeTree(pathList);
		assert(tree.size() == 2);
		assert(tree[0]->getPath() == "/root");
		assert(tree[1]->getPath() == "/root/child");
		assert(tree[0]->getChildren()[0] == tree[1].get());
	}
	
	{
		rte::ParamUpdateCommand c, c1;
		c.add(&param1);
		c.add(&param2);
		c.add(&param3);

		rte::NodeParameter param4("param4", root, rte::ParameterType::File);
		{
			rte::File f;
			strcpy_s(f.path, "D:\\tmp\\test.py");
			param4.setValue(f);
		}
		c.add(&param4);

		rte::NodeParameter::setFileParameterRoot("D:\\tmp");
		c.serialize(buffer.get(), &context);

		rte::NodeParameter::setFileParameterRoot("D:\\tmp\\test");
		c1.deserialize(buffer.get(), &context);

		auto list = c1.getParameterList();
		assert(list.size() == 4);
		assert(list[0]->getValue<int32_t>() == 1);
		assert(list[0]->getName() == "param1");
		assert(list[0]->getPath() == "/root/param1");
		assert(list[1]->getValue<int32_t>() == 2);
		assert(list[1]->getName() == "param2");
		assert(list[1]->getPath() == "/root/param2");
		assert(list[2]->getValue<std::string>() == "abcde");
		assert(list[2]->getName() == "param3");
		assert(list[2]->getPath() == "/root/param3");
		assert(list[3]->getName() == "param4");
		assert(list[3]->getPath() == "/root/param4");
	}
	{
		rte::Socket::setup();
		rte::Socket::shutdown();
	}

	return 0;
}

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	int result = _main(argc, argv);
	return result;
}

#endif// #ifndef _SWIG_PY
