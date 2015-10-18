#include "Command.h"

#include "Node.h"
#include "NodeParameter.h"
#include <algorithm>
#include <vector>
#include <cassert>

namespace {
	size_t getHeaderSize()
	{
		return 0;
	}

	uint8_t* readHeader(uint8_t* buffer)
	{
		return static_cast<uint8_t*>(buffer);
	}

	uint8_t* writeHeader(uint8_t* buffer)
	{
		return static_cast<uint8_t*>(buffer);
	}

	
	uint8_t* readType(rte::CommandType* pOut, uint8_t* buffer)
	{
		memcpy(pOut, buffer, sizeof(rte::CommandType));
		return static_cast<uint8_t*>(buffer)+sizeof(rte::CommandType);
	}

	uint8_t* writeType(uint8_t* buffer, rte::CommandType value)
	{
		memcpy(buffer, &value, sizeof(rte::CommandType));
		return static_cast<uint8_t*>(buffer)+sizeof(rte::CommandType);
	}

	uint8_t* readInt32(int32_t* pOut, uint8_t* buffer)
	{
		memcpy(pOut, buffer, sizeof(int32_t));
		return static_cast<uint8_t*>(buffer)+sizeof(int32_t);
	}

	uint8_t* writeInt32(uint8_t* buffer, int32_t value)
	{
		memcpy(buffer, &value, sizeof(int32_t));
		return static_cast<uint8_t*>(buffer) + sizeof(int32_t);
	}

	uint8_t* readUInt32(uint32_t* pOut, uint8_t* buffer)
	{
		memcpy(pOut, buffer, sizeof(uint32_t));
		return static_cast<uint8_t*>(buffer)+sizeof(uint32_t);
	}

	uint8_t* writeUInt32(uint8_t* buffer, uint32_t value)
	{
		memcpy(buffer, &value, sizeof(uint32_t));
		return static_cast<uint8_t*>(buffer)+sizeof(uint32_t);
	}

	uint8_t* writeString(uint8_t* buffer, const std::string& value)
	{
		memcpy(buffer, value.c_str(), value.length());
		return static_cast<uint8_t*>(buffer) + value.length();
	}
}

namespace rte {

	/*------------------------------------------------------------------*/

	PingCommand::PingCommand()
		: Command(CommandType::Ping)
	{}

	/*
	- header
	- type: CommandType
	*/
	uint8_t* PingCommand::serialize(uint8_t* buffer, Context*)
	{
		uint8_t* ptr = readHeader(buffer);
		ptr = readType(&mType, ptr);
		return ptr;
	}

	uint8_t* PingCommand::deserialize(uint8_t* buffer, Context*)
	{
		uint8_t* ptr = writeHeader(buffer);
		ptr = writeType(ptr, mType);
		return ptr;
	}

	/*------------------------------------------------------------------*/

#ifdef _SWIG_PY
	std::vector<Node*> NodeNameListCommand::createNodeTree(const std::vector<std::string>& pathList)
	{
		std::vector<Node*> result;
#else
	std::vector<std::unique_ptr<Node>> NodeNameListCommand::createNodeTree(const std::vector<std::string>& pathList)
	{
		std::vector<std::unique_ptr<Node>> result;
#endif

		if (pathList.size() == 0)
		{
			return result;
		}

		for (const std::string& path : pathList)
		{
			const std::string& parent = path::dirname(path);
			if (parent.size() == 0)
			{
#ifdef _SWIG_PY
				result.push_back(new Node(path::basename(path), nullptr));
#else
				result.push_back(std::move(std::unique_ptr<Node>(new Node(path::basename(path), nullptr))));
#endif
			}
			else
			{
				bool foundParent = false;
				for (auto& pNode : result)
				{
					if (pNode->getPath() == parent)
					{
#ifdef _SWIG_PY
						result.push_back(new Node(path::basename(path), pNode));
#else
						result.push_back(std::move(std::unique_ptr<Node>(new Node(path::basename(path), pNode.get()))));
#endif
						foundParent = true;
						break;
					}
				}
				if (!foundParent)
				{
					std::cerr << "[NodeNameListCommand::createNodeTree] parent not found: " << path << std::endl;
				}
			}
		}

		#undef PUSH_RESULT

#ifdef _SWIG_PY
		return result;
#else
		return std::move(result);
#endif
	}

	NodeNameListCommand::NodeNameListCommand()
		: Command(CommandType::NodeNameList)
	{ }

	/*
	 - header
	 - type: CommandType
	 - size: int32, names size
	 - names: name string joined by ','
	 */
	uint8_t* NodeNameListCommand::serialize(uint8_t* buffer, Context*)
	{
		uint8_t* ptr = writeHeader(buffer);
		ptr = readType(&mType, ptr);

		std::sort(mPathList.begin(), mPathList.end());

		int32_t size = 0;
		for (auto path : mPathList)
		{
			size += path.length() + 1;
		}
		if (mPathList.size() > 0)
		{
			size -= 1;
		}
		ptr = writeInt32(ptr, size);

		std::string tmp;
		string::join(&tmp, mPathList, ',');
		ptr = writeString(ptr, tmp);

		assert(ptr == static_cast<uint8_t*>(buffer)+getHeaderSize() + sizeof(CommandType) + sizeof(int32_t) + size);
		return ptr;
	}

	uint8_t* NodeNameListCommand::deserialize(uint8_t* buffer, Context*)
	{
		uint8_t* ptr = readHeader(buffer);
		ptr = writeType(ptr, mType);

		int32_t size;
		ptr = readInt32(&size, ptr);

		std::sort(mPathList.begin(), mPathList.end());

		char* tmp = new char[size + 1];
		{
			memcpy(tmp, ptr, size);
			ptr += size;
			tmp[size] = '\0';
			string::split(&mPathList, tmp, ',');
		}
		mem::safeDeleteArray(&tmp);

		assert(ptr == static_cast<uint8_t*>(buffer)+getHeaderSize() + sizeof(CommandType) + sizeof(int32_t) + size);
		return ptr;
	}

	void NodeNameListCommand::add(const Node* pNode)
	{
		mPathList.push_back(pNode->getPath());
	}

	/*------------------------------------------------------------------*/

	NodeCommand::NodeCommand()
		: Command(CommandType::Node)
		, mpNode(nullptr), mOwnNode(false)
	{ }

	NodeCommand::~NodeCommand()
	{
		if (mOwnNode)
		{
			mem::safeDelete(&mpNode);
		}
	}

	/*
	- header
	- type: CommandType
	- [Node]
	*/
	uint8_t* NodeCommand::serialize(uint8_t* buffer, Context* pContext)
	{
		assert(mpNode != nullptr && "serialisz NULL node");

		uint8_t* ptr = readHeader(buffer);
		ptr = writeType(ptr, mType);

		return mpNode->serialize(ptr, pContext);
	}

	uint8_t* NodeCommand::deserialize(uint8_t* buffer, Context* pContext)
	{
		uint8_t* ptr = writeHeader(buffer);
		ptr = readType(&mType, ptr);

		if (mOwnNode && mpNode != nullptr)
		{
			mem::safeDelete(&mpNode);
		}
		mpNode = new Node();
		mOwnNode = true;
		return mpNode->deserialize(ptr, pContext);
	}

	Node* NodeCommand::get()
	{
		return mpNode;
	}

	void NodeCommand::set(Node* pNode)
	{
		if (mOwnNode && mpNode != nullptr)
		{
			mem::safeDelete(&mpNode);
		}
		mpNode = pNode;
		mOwnNode = false;
	}

	/*------------------------------------------------------------------*/

	ParamUpdateCommand::ParamUpdateCommand()
		: Command(CommandType::ParamUpdate)
		, mOwnParam(false)
	{ }

	ParamUpdateCommand::~ParamUpdateCommand()
	{
		if (mOwnParam)
		{
			for (NodeParameter* pParam : mParamPtrList)
			{
				mem::safeDelete(&pParam);
			}
		}
	}

	/*
	- header
	- type: CommandType
	- count: uint32_t, parameter count
	- [Parameters]
	*/
	uint8_t* ParamUpdateCommand::serialize(uint8_t* buffer, Context* pContext)
	{
		assert(mOwnParam == false && "cannot serialize the node that owns parameters");

		uint8_t* ptr = writeHeader(buffer);
		ptr = writeType(ptr, mType);

		const uint32_t count = mParamPtrList.size();
		ptr = writeUInt32(ptr, count);

		for (NodeParameter* pParam : mParamPtrList)
		{
			ptr = pParam->serialize(ptr, pContext);
		}

		return ptr;
	}

	uint8_t* ParamUpdateCommand::deserialize(uint8_t* buffer, Context* pContext)
	{
		if (mOwnParam)
		{
			for (NodeParameter* pParam : mParamPtrList)
			{
				mem::safeDelete(&pParam);
			}
		}
		mParamPtrList.clear();

		uint8_t* ptr = readHeader(buffer);
		ptr = readType(&mType, ptr);

		uint32_t count;
		ptr = readUInt32(&count, ptr);

		for (int i = 0; i < static_cast<int>(count); ++i)
		{
			NodeParameter* pParam = new NodeParameter();
			ptr = pParam->deserialize(ptr, pContext);
			mParamPtrList.push_back(pParam);
		}
		mOwnParam = true;

		return ptr;
	}

	void ParamUpdateCommand::add(NodeParameter* pNodeParam)
	{
		assert(mOwnParam == false && "cannot add parameters to the deserialized node that owns parameters");
		mParamPtrList.push_back(pNodeParam);
	}

	const std::vector<NodeParameter*>& ParamUpdateCommand::getParameterList()
	{
		return mParamPtrList;
	}

}// namespace rte
