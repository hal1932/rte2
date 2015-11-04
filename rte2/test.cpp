#ifndef _SWIG_PY

#include "Node.h"
#include "NodeParameter.h"
#include "Context.h"
#include "Command.h"
#include "Socket.h"
#include "TcpServer.h"
#include "TcpClient.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <map>
#include <iostream>
#include <memory>

#if false
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
#else
#if true
int _main(int argc, char* argv[])
{
	rte::Socket::setup();

	volatile bool closed = false;
	rte::TcpServer server;

	server.open(0x1234);

	while (true)
	{
		auto accepted = server.popAcceptedQueue();
		for (auto client : accepted)
		{
			std::cout << "accept: " << client << std::endl;

			auto clientCount = server.getClientCount();
			if (clientCount > 0)
			{
				auto clients = server.getClientList();
				for (auto client : clients)
				{
					std::cout << "client: " << client << std::endl;
				}
			}
		}

		auto received = server.popReceivedQueue();
		for (auto data : received)
		{
			if (data.bufferSize > 0)
			{
				std::cout << "receive: " << data.clientId << ", " << data.buffer << ", " << data.bufferSize << std::endl;
				server.sendAsync(data.clientId, data.buffer, data.bufferSize);// echo back
			}
			else
			{
				std::cout << "receive error: " << data.clientId << std::endl;
				server.closeConnection(data.clientId);
				data.destroy();
			}
		}

		auto sent = server.popSentQueue();
		for (auto data : sent)
		{
			if (data.bufferSize == data.sentSize)
			{
				std::cout << "sent: " << data.clientId << ", " << data.buffer << ". " << data.bufferSize << std::endl;
			}
			else
			{
				std::cout << "sent error: " << data.clientId << ", " << data.buffer << ". " << data.bufferSize << std::endl;
				server.closeConnection(data.clientId);
			}
			data.destroy();
		}

		auto closed = server.popClosedQueue();
		for (auto client : closed)
		{
			std::cout << "closed: " << client << std::endl;
		}

		Sleep(10);
	}

	server.close();
	rte::Socket::shutdown();

	return 0;
}
#else
int _main(int argc, char* argv[])
{
	rte::Socket::setup();

	rte::TcpClientConfig config;
	config.OnSendData = [](const uint8_t* buffer, int bufferSize)
	{
		std::cout << "send: " << std::string((char*)buffer, bufferSize) << std::endl;
	};
	config.OnReceiveData = [](const uint8_t* buffer, int bufferSize)
	{
		std::cout << "receive: " << std::string((char*)buffer, bufferSize) << std::endl;
	};
	config.OnConnectionError = [](const uint8_t* buffer, int bufferSize)
	{
		std::cout << "error: " << std::string((char*)buffer, bufferSize) << std::endl;
		return false;
	};

	rte::mem::SafeArray<uint8_t> data(4);
	memcpy(data.get(), "1234", 4);

	rte::TcpClient client;
	std::cout << "configure: " << client.configure(&config) << std::endl;
	std::cout << "connect: " << client.connect("127.0.0.1", 0x1234) << std::endl;
	client.sendAsync(data.get(), data.size());

	while (true)
	{
		Sleep(1);
	}

	client.close();
	rte::Socket::shutdown();
	return 0;
}
#endif
#endif


int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	int result = _main(argc, argv);
	return result;
}

#endif// #ifndef _SWIG_PY
