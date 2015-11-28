#ifndef _SWIG_PY

#include "common.h"
#include "core.h"
#include "Node.h"
#include "Socket.h"
#include "TcpServer.h"
#include "TcpClient.h"
#include "NodeContent.h"
#include "NodeSerializationContext.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <map>
#include <iostream>
#include <memory>

void dump(uint8_t* buffer, int size)
{
	for (auto i = 0; i < size; ++i)
	{
		printf("%X ", buffer[i]);
	}
	puts("");

}

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
#if false
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
				printf("receive: %d, ", data.clientId);
				dump(data.buffer, data.bufferSize);

				server.sendAsync(data.clientId, data.buffer, data.bufferSize);// echo back
			}
			else
			{
				printf("receive error: %d", data.clientId);

				server.closeConnection(data.clientId);
				data.destroy();
			}
		}

		auto sent = server.popSentQueue();
		for (auto data : sent)
		{
			if (data.bufferSize == data.sentSize)
			{
				printf("sent: %d, ", data.clientId);
				dump(data.buffer, data.bufferSize);
			}
			else
			{
				printf("sent error: %d, ", data.clientId);
				dump(data.buffer, data.bufferSize);

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
#if false
int _main(int argc, char* argv[])
{
	rte::Socket::setup();

	rte::mem::Array<uint8_t> item(4);
	memcpy(item.get(), "1234", 4);

	rte::TcpClient client;
	std::cout << "connect: " << client.connect("127.0.0.1", 0x1234) << std::endl;
	client.sendAsync(item.get(), item.size());

	bool closed = false;

	while (true)
	{
		auto sent = client.popSentQueue();
		for (auto data : sent)
		{
			if (data.bufferSize == data.sentSize)
			{
				printf("sent: ");
				dump(data.buffer, data.bufferSize);
			}
			else
			{
				printf("sent error: ");
				dump(data.buffer, data.bufferSize);
				closed = true;
			}
			data.deallocate();
		}

		auto received = client.popReceivedQueue();
		for (auto data : received)
		{
			closed = true;

			if (data.bufferSize > 0)
			{
				printf("receive: ");
				dump(data.buffer, data.bufferSize);
			}
			else
			{
				printf("receive error");
				break;
			}
			data.deallocate();
		}

		if (closed)
		{
			break;
		}

		Sleep(1);
	}

	client.close();
	rte::Socket::shutdown();
	return 0;
}
#else
#if true
int _main(int, char**)
{
	rte::core::setup();

	auto pRootNode = rte::Node::createRootNode("root", "root_label");

	auto pChild1 = pRootNode->addChild("child1", "child1_label");
	auto pContent1 = pChild1->createContent("content1", "content1_label");
	auto pData1 = pContent1->createData<rte::Int32Data>();
	pData1->Value = 1234;

	rte::mem::SafeArray<uint8_t> buffer(pRootNode->calcSize());
	pRootNode->serialize(buffer.get());

	rte::TcpClient client;
	client.connect("127.0.0.1", 0x1234);
	client.sendAsync(buffer.get(), buffer.size());

	// send
	while (true)
	{
		if (client.popSentQueue().size() > 0)
		{
			break;
		}
		Sleep(10);
	}

	// receive, deserialize
	while (true)
	{
		if (!client.isConnectionAlive())
		{
			std::cout << "connection is closed by server" << std::endl;
			break;
		}

		auto received = client.popReceivedQueue();
		if (received.size() > 0)
		{
			assert(received.size() == 1);
			auto data = received[0];

			rte::NodeDeserializationContext context(data.buffer, data.bufferSize);
			while (context.hasNext())
			{
				auto pn = context.getNext();
				std::cout << pn->getName() << std::endl;
				rte::Node::destroy(&pn);
			}
			data.deallocate();

			std::cout << "success!" << std::endl;

			break;
		}
		Sleep(10);
	}

	rte::Node::destroy(&pRootNode);

	client.close();
	rte::core::shutdown();
	return 0;
}
#else
int _main(int, char**)
{
	rte::core::setup();

	rte::TcpServer server;
	server.open(0x1234);
	server.setKeepAliveInterval(1);

	while (true)
	{
		if (server.cleanupInvalidConnection())
		{
			auto clientCount = server.getClientCount();
			printf("client count: %d\n", clientCount);
			if (clientCount == 0)
			{
				break;
			}
		}

		auto clients = server.popAcceptedQueue();
		for (auto c : clients)
		{
			printf("accept: %d\n", c);
		}

		auto received = server.popReceivedQueue();
		for (auto r : received)
		{
			if (r.bufferSize > 0)
			{
				printf("received from %d\n", r.clientId);
				// echo back
				server.sendAsync(r.clientId, r.buffer, r.bufferSize);
			}
		}

		auto sents = server.popSentQueue();
		for (auto s : sents)
		{
			printf("sent to %d, %d\n", s.clientId, s.sentSize);
			rte::mem::safeDelete(&s.buffer);
		}

		auto closed = server.popClosedQueue();
		for (auto c : closed)
		{
			printf("closed %d\n", c);
		}

		Sleep(10);
	}

	server.close();
	rte::core::shutdown();

	return 0;
}
#endif
#endif
#endif
#endif


int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	int result = _main(argc, argv);
	return result;
}

#endif// #ifndef _SWIG_PY
