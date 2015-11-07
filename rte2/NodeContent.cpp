#include "NodeContent.h"
#include "Node.h"

namespace rte
{

	NodeContent::NodeContent(Node* pOwnerNode)
		: mpOwnerNode(pOwnerNode),
		  mName(std::to_string(math::xor128()))
	{
		updatePath();
	}

	void NodeContent::setName(const std::string& name)
	{
		mName = name;
		updatePath();
	}

	void NodeContent::updatePath()
	{
		mPath = path::combine(mpOwnerNode->getPath(), mName);
	}

}// namespace rte
