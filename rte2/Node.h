#pragma once
#include "common.h"
#include <string>
#include <vector>
#include <algorithm>

namespace rte
{
	class NodeContent;

	class Node : private noncopyable, private nonmovable
	{
	public:
		Node(Node* pParent);
		Node(const std::string& name, const std::string& label, Node* pParent);
		~Node();

#ifdef _SWIG_PY
		// C++ポインタ比較の代わりに定義しておく
		// SWIGではSwigObject*同士で等値判定をするので、同じ場所を指すポインタ的に同じでもSWIG越しだと違ってみえる
		// 同じパスを持つノードは存在しないことをaddChild()で保証しているので、SWIGのみパスで等値判定を行う
		bool operator==(const Node& other)
		{
			return other.getPath() == getPath();
		}
#else
		// 同じノード実体が複数存在する状況を許容しないので、実体の等値比較は必要ない
		bool operator==(const Node&) = delete;
#endif

		Node() = delete;

		const std::string& getName() { return mName; }
		const std::string& getLabel() { return mLabel; }
		const std::string& getPath() { return mPath; }

		const std::string& getName() const { return mName; }
		const std::string& getLabel() const { return mLabel; }
		const std::string& getPath() const { return mPath; }

		Node* getParent() { return mpParent; }
		std::vector<Node*> getChildren() { return mChildPtrList; }

		void createContent();
		NodeContent* getContent() { return mpContent; }

		void setName(const std::string& name);
		void setLabel(const std::string& label) { mLabel = label; }

		void addChild(Node* pChild)
		{
			assert(pChild != nullptr);
			assert(findChild(pChild) == nullptr);
			assert(findChild(pChild->getName()) == nullptr);
			mChildPtrList.push_back(pChild);
		}

		Node* findChild(const std::string& name);
		Node* findChild(Node* pChild);

		Node* removeChild(const std::string& name);
		Node* removeChild(const Node* pNode);

	private:
		std::string mName;
		std::string mLabel;
		std::string mPath;

		NodeContent* mpContent;

		Node* mpParent;
		std::vector<Node*> mChildPtrList;

		void updatePath();
	};

}//namespace rte
