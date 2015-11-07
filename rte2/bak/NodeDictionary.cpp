#include "NodeDictionary.h"
#include "Node.h"
#include <cassert>

namespace rte {
	
	NodeDictionary::~NodeDictionary()
	{
		assert(empty());
	}

	void NodeDictionary::add(NodeBase* pNode)
	{
		const char* path = pNode->getPath().c_str();
		if (!mDic.empty())
		{
			assert(mDic.find(path) == mDic.end() && "the node that has same path is already exists");
		}
		mDic[path] = pNode;
	}

	void NodeDictionary::remove(NodeBase* pNode)
	{
		const char* path = pNode->getPath().c_str();
		auto found = mDic.find(path);
		assert(found != mDic.end() && "cannot remove the node that is not exists or already removed");
		mDic.erase(found);
	}

	NodeBase* NodeDictionary::find(const std::string& path)
	{
		auto found = mDic.find(path.c_str());
		return (found == mDic.end()) ? nullptr : found->second;
	}

	bool NodeDictionary::empty()
	{
		return mDic.empty();
	}

	void NodeDictionary::clear(void(*deleter)(NodeBase**))
	{
		for (auto item : mDic)
		{
			deleter(&item.second);
		}
		mDic.clear();
	}

}// namespace rte
