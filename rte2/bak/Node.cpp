#include "Node.h"
#include "NodeParameter.h"
#include "memutil.h"
#include "Context.h"

namespace rte {

	Node::Node()
		: NodeBase("", nullptr), mOwnParam(false)
	{ }

	Node::Node(const std::string& name, Node* pParent)
		: NodeBase(name, pParent), mOwnParam(false)
	{
		if (pParent != nullptr)
		{
			pParent->mChildPtrList.push_back(this);
		}
	}

	Node::Node(Node&& other)
		: NodeBase(other.getName(), other.getParent()), mOwnParam(other.mOwnParam)
	{
		mChildPtrList = std::move(other.mChildPtrList);
		mParamPtrList = std::move(other.mParamPtrList);
		other.~Node();
	}

	Node::~Node()
	{
		if (mOwnParam)
		{
			for (NodeParameter* pParam : mParamPtrList)
			{
				mem::safeDelete(&pParam);
			}
			mParamPtrList.clear();
		}
	}

	std::vector<Node*> Node::getChildren()
	{
		return mChildPtrList;
	}

	const std::vector<NodeParameter*>& Node::getParameterList()
	{
		return mParamPtrList;
	}

	void Node::addParemeter(NodeParameter* pParam)
	{
		assert(!mOwnParam && "cannot add params to the deserialized node");
		pParam->setParent(this);
		mParamPtrList.push_back(pParam);
	}

	uint8_t* Node::serialize(uint8_t* buffer, Context* pContext)
	{
		uint8_t* ptr = static_cast<uint8_t*>(buffer);
		ptr = writeString(getPath(), ptr);

		ptr = writeUInt32(mParamPtrList.size(), ptr);
		for (NodeParameter* pParam : mParamPtrList)
		{
			ptr = pParam->serialize(ptr, pContext);
		}

		return ptr;
	}

	uint8_t* Node::deserialize(uint8_t* buffer, Context* pContext)
	{
		uint8_t* ptr = static_cast<uint8_t*>(buffer);

		std::string path;
		ptr = readString(&path, ptr);
		setPath(path, pContext);

		uint32_t paramCount;
		ptr = readUInt32(&paramCount, ptr);

		if (mOwnParam)
		{
			for (NodeParameter* pParam : mParamPtrList)
			{
				mem::safeDelete(&pParam);
			}
		}
		mParamPtrList.clear();

		for (int i = 0; i < static_cast<int>(paramCount); ++i)
		{
			NodeParameter* pParam = new NodeParameter();
			ptr = pParam->deserialize(ptr, pContext);
			mParamPtrList.push_back(pParam);
		}
		mOwnParam = true;
		
		return ptr;
	}

}// namespace rte
