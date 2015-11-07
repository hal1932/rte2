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
		// C++�|�C���^��r�̑���ɒ�`���Ă���
		// SWIG�ł�SwigObject*���m�œ��l���������̂ŁA�����ꏊ���w���|�C���^�I�ɓ����ł�SWIG�z�����ƈ���Ă݂���
		// �����p�X�����m�[�h�͑��݂��Ȃ����Ƃ�addChild()�ŕۏ؂��Ă���̂ŁASWIG�̂݃p�X�œ��l������s��
		bool operator==(const Node& other)
		{
			return other.getPath() == getPath();
		}
#else
		// �����m�[�h���̂��������݂���󋵂����e���Ȃ��̂ŁA���̂̓��l��r�͕K�v�Ȃ�
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
