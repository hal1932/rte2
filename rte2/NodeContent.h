#pragma once
#include "common.h"
#include <string>

namespace rte
{
	class Node;

	class NodeContent
	{
	public:
		explicit NodeContent(Node* pOwnerNode);

		NodeContent(NodeContent&& other) {}
		NodeContent& operator=(NodeContent&& other) {}
		~NodeContent() {}

		NodeContent() = delete;

		const std::string& getName() { return mName; }
		const std::string& getPath() { return mPath; }
		const std::string& getLabel() { return mLabel; }

		void setName(const std::string& name);
		void setLabel(const std::string& label) { mLabel = label; }

	private:
		Node* mpOwnerNode;

		std::string mName;
		std::string mPath;

		std::string mLabel;

		void updatePath();
	};

}// namespace rte
