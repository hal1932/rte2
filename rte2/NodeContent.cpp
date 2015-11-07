#include "NodeContent.h"
#include "Node.h"
#include <cstdlib>

namespace rte
{

	NodeContent::NodeContent(Node* pOwnerNode)
		: mpOwnerNode(pOwnerNode)
	{
		char tmp[8 + 1];
		do
		{
			_itoa_s(math::xor128(), tmp, 16);
		} while (mName == tmp);
		mName = tmp;

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
