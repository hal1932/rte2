#pragma once
#include "common.h"
#include "NodeBase.h"
#include <string>
#include <memory>

namespace rte
	{

	class NodeParameter;
	class Context;

	class Node RTE_FINAL : public NodeBase
	{
	public:
		Node();
		Node(const std::string& name, Node* pParent);
		Node(Node&& other);

		~Node();

		std::vector<Node*> getChildren();
		const std::vector<NodeParameter*>& getParameterList();

		void addParemeter(NodeParameter* pParam);

		uint8_t* serialize(uint8_t* buffer, Context* pContext);
		uint8_t* deserialize(uint8_t* buffer, Context* pContext);

	private:
		std::vector<Node*> mChildPtrList;
		std::vector<NodeParameter*> mParamPtrList;
		bool mOwnParam;
	};
}// namespace rte
