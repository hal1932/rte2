#pragma once
#include "common.h"
#include <vector>

namespace rte {

	class Node;
	class NodeContent;

	class NodeSerializationContext : private noncopyable, private nonmovable
	{
	public:
		NodeSerializationContext();
		~NodeSerializationContext() = default;

		void addNode(Node* pNode) { mNodePtrList.push_back(pNode); }
		void addContent(NodeContent* pContent) { mContentPtrList.push_back(pContent); }

		const mem::Array<uint8_t>& serialize();

	private:
		std::vector<Node*> mNodePtrList;
		std::vector<NodeContent*> mContentPtrList;
		mem::Array<uint8_t> mBuffer;
	};

	class NodeDeserializationContext : private noncopyable, private nonmovable
	{
	public:
		NodeDeserializationContext(uint8_t* buffer, int bufferSize);
		~NodeDeserializationContext() = default;

		void deserialize();
		const std::vector<Node*>& getNodeList() { return mNodePtrList; }
		const std::vector<NodeContent*>& getContentPtrList() { return mContentPtrList; }

	private:
		uint8_t* mpBuffer;
		int mBufferSize;

		std::vector<Node*> mNodePtrList;
		std::vector<NodeContent*> mContentPtrList;
	};

}// namespace rte
