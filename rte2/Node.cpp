#include "Node.h"
#include "NodeContent.h"
#include "memutil.h"

namespace rte
{
	Node* Node::createRootNode(const std::string& name, const std::string& label)
	{
		auto pRoot = new Node(nullptr);
		pRoot->setName(name);
		pRoot->setLabel(label);
		return pRoot;
	}

	void Node::destroy(Node** ppNode)
	{
		auto pNode = *ppNode;
		if (pNode == nullptr)
		{
			return;
		}

		mem::safeDelete(&pNode->mpContent);

		for (auto pChild : pNode->mChildPtrList)
		{
			destroy(&pChild);
		}
		pNode->mChildPtrList.clear();

		delete pNode;
		pNode = nullptr;
	}

	void Node::setName(const std::string& name)
	{
		mName = name;
		updatePath();
	}

	NodeContent* Node::createContent(const std::string& name, const std::string& label)
	{
		mem::safeDelete(&mpContent);
		mpContent = new NodeContent(this);

		if (name.length() > 0)
		{
			mpContent->setName(name);
		}
		if(label.length() > 0)
		{
			mpContent->setLabel(label);
		}

		return mpContent;
	}

	Node* Node::addChild(const std::string& name, const std::string& label)
	{
		assert(findChild(name) == nullptr);

		auto pChild = createChild();
		pChild->setName(name);
		pChild->setLabel(label);

		mChildPtrList.push_back(pChild);
		++mChildCount;

		assert(mChildCount == static_cast<int32_t>(mChildPtrList.size()));

		return pChild;
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

	bool Node::removeChild(const std::string& name)
	{
		for (auto iter = mChildPtrList.begin(); iter != mChildPtrList.end(); ++iter)
		{
			if ((*iter)->getName() == name)
			{
				mChildPtrList.erase(iter);
				Node::destroy(&(*iter));
				return true;
			}
		}
		return false;
	}

	bool Node::removeChild(const Node* pChild)
	{
		if (pChild != nullptr)
		{
			for (auto iter = mChildPtrList.begin(); iter != mChildPtrList.end(); ++iter)
			{
				if ((*iter) == pChild)
				{
					mChildPtrList.erase(iter);
					Node::destroy(&(*iter));
					return true;
				}
			}
		}
		return false;
	}

	int Node::calcSize(int depth)
	{
		int size = 0;

		// 名前、ラベル
		size += calcSizeString(mName);
		size += calcSizeString(mLabel);

		// Content
		size += calcSizeInt32(int32_t());// content size
		if (mpContent != nullptr)
		{
			size += mpContent->calcSize();
		}

		// 子供
		size += calcSizeInt32(int32_t());// children size
		if (depth > 0)
		{
			for (auto pChild : mChildPtrList)
			{
				size += pChild->calcSize(depth - 1);
			}
		}

		return size;
	}

	uint8_t* Node::serialize(uint8_t* buffer, int depth)
	{
		auto ptr = buffer;

		// 名前、ラベル
		ptr = writeString(mName, ptr);
		ptr = writeString(mLabel, ptr);

		// Content
		{
			if (mpContent != nullptr)
			{
				auto contentSize = static_cast<int32_t>(mpContent->calcSize());
				ptr = writeInt32(contentSize, ptr);
				ptr = mpContent->serialize(ptr);
			}
			else
			{
				ptr = writeInt32(0, ptr);
			}
		}

		// 子供
		{
			if (depth > 0)
			{
				ptr = writeInt32(mChildCount, ptr);

				for (auto i = 0; i < mChildCount; ++i)
				{
					ptr = mChildPtrList[i]->serialize(ptr, depth - 1);
				}
			}
			else
			{
				ptr = writeInt32(0, ptr);
			}
		}

		assert(ptr == buffer + calcSize());

		return ptr;
	}

	uint8_t* Node::deserialize(uint8_t* buffer, int depth)
	{
		auto ptr = buffer;

		// 名前、ラベル
		ptr = readString(&mName, ptr);
		ptr = readString(&mLabel, ptr);
		updatePath();

		// Content
		{
			int32_t contentSize;
			ptr = readInt32(&contentSize, ptr);
			if (contentSize > 0)
			{
				ptr = createContent()->deserialize(ptr);
			}
			else
			{
				mem::safeDelete(&mpContent);
			}
		}

		// 子供
		{
			for (auto pChild : mChildPtrList)
			{
				destroy(&pChild);
			}
			mChildPtrList.clear();

			ptr = readInt32(&mChildCount, ptr);

			if (depth > 0)
			{
				for (auto i = 0; i < mChildCount; ++i)
				{
					auto pChild = createChild();
					ptr = pChild->deserialize(ptr, depth - 1);
					mChildPtrList.push_back(pChild);
				}
			}
		}

		assert(ptr == buffer + calcSize());

		return ptr;
	}

	Node::Node(Node* pParent)
		: mpParent(pParent)
	{ }

	Node* Node::createChild()
	{
		return new Node(this);
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

		if (mpContent != nullptr)
		{
			mpContent->updatePath();
		}

		for (auto pChild : mChildPtrList)
		{
			pChild->updatePath();
		}
	}

}// namespace rte
