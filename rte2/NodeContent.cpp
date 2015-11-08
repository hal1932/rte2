#include "NodeContent.h"
#include "Node.h"

namespace rte
{

	NodeContent::NodeContent(Node* pOwnerNode)
		: mpOwnerNode(pOwnerNode),
		  mName(std::to_string(math::xor128())),
		  mpData(nullptr)
	{
		updatePath();
	}

	NodeContent::~NodeContent()
	{
		mem::safeDelete(&mpData);
	}

	void NodeContent::setName(const std::string& name)
	{
		mName = name;
		updatePath();
	}

	int NodeContent::calcSize()
	{
		throw new std::exception("not implemented");
	}

	uint8_t* NodeContent::serialize(uint8_t* buffer)
	{
		throw new std::exception("not implemented");
	}

	uint8_t* NodeContent::deserialize(uint8_t* buffer)
	{
		throw new std::exception("not implemented");
	}

	void NodeContent::updatePath()
	{
		mPath = path::combine(mpOwnerNode->getPath(), mName);
	}

}// namespace rte
