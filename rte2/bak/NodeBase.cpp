#include "NodeBase.h"
#include "Context.h"
#include "Node.h"

namespace rte {

	NodeBase::NodeBase(const std::string& name, NodeBase* pParent)
		: mName(name), mpParent(pParent)
	{
		updatePath();
	}

	void NodeBase::setParent(NodeBase* pParent)
	{
		mpParent = pParent;
		updatePath();
	}

	void NodeBase::setPath(const std::string& path, Context* pContext)
	{
		mPath = path;
		mName = path::basename(path);
		mpParent = pContext->findNode(path::dirname(path));
	}

	void NodeBase::updatePath()
	{
		if (mpParent != nullptr)
		{
			mPath = path::combine(mpParent->getPath(), mName);
		}
		else
		{
			mPath = "/" + mName;
		}
	}

}// namespace rte
