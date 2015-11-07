#include "Node.h"

namespace rte
{

	void Node::setName(const std::string& name)
	{
		mName = name;
		updatePath();
		for (auto pChild : mChildPtrList)
		{
			pChild->updatePath();
		}
	}

	Node* Node::findChild(const std::string& name)
	{
		auto found = std::find_if(
			mChildPtrList.begin(), mChildPtrList.end(),
			[&name](Node* pChild) { return pChild->getName() == name; });
		return (found != mChildPtrList.end()) ? *found : nullptr;
	}

	Node* Node::findChild(Node* pChild)
	{
		auto found = std::find(mChildPtrList.begin(), mChildPtrList.end(), pChild);
		return (found != mChildPtrList.end()) ? *found : nullptr;
	}

	Node* Node::removeChild(const std::string& name)
	{
		for (auto iter = mChildPtrList.begin(); iter != mChildPtrList.end(); ++iter)
		{
			if ((*iter)->getName() == name)
			{
				return *mChildPtrList.erase(iter);
			}
		}
		return nullptr;
	}

	Node* Node::removeChild(const Node* pNode)
	{
		if (pNode != nullptr)
		{
			for (auto iter = mChildPtrList.begin(); iter != mChildPtrList.end(); ++iter)
			{
				if ((*iter) == pNode)
				{
					return *mChildPtrList.erase(iter);
				}
			}
		}
		return nullptr;
	}

	void Node::updatePath()
	{
		if (mpParent != nullptr)
		{
			mPath = path::combine(mpParent->getPath(), mName);
		}
		else
		{
			mPath = mName;
		}
	}

}// namespace rte
