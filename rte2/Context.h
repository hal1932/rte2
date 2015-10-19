#pragma once
#include "common.h"
#include "NodeDictionary.h"

namespace rte
	{

	class Node;

	class Context : noncopyable, nonmovable
	{
	public:
		Context();
		~Context();

		Node* addNode(const std::string& name, Node* pParent);
		Node* findNode(const std::string& path);
		void removeNode(Node* pNode);

	private:
		NodeDictionary mNodeDic;
	};

}// namespace rte
