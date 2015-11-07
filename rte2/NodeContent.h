#pragma once
#include "common.h"

namespace rte
{
	class Node;

	class NodeContent
	{
	public:
		explicit NodeContent(Node* pOwnerNode)
			: mpOwnerNode(pOwnerNode)
		{ }

		NodeContent(NodeContent&& other) {}
		NodeContent& operator=(NodeContent&& other) {}
		~NodeContent() {}

		NodeContent() = delete;

	private:
		Node* mpOwnerNode;
	};

}// namespace rte
