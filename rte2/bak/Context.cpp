#include "Context.h"
#include "Node.h"

namespace rte {

	Context::Context()
	{ }

	Context::~Context()
	{
		mNodeDic.clear(mem::safeDelete);
	}

	Node* Context::addNode(const std::string& name, Node* pParent)
	{
		Node* pNode = new Node(name, pParent);
		mNodeDic.add(pNode);
		return pNode;
	}

	Node* Context::findNode(const std::string& path)
	{
		return static_cast<Node*>(mNodeDic.find(path));
	}

	void Context::removeNode(Node* pNode)
	{
		mNodeDic.remove(pNode);
		mem::safeDelete(&pNode);
	}

}// namespace rte
