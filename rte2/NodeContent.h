#pragma once
#include "common.h"
#include "NodeContentData.h"
#include <string>

namespace rte
{
	class Node;
	class NodeContentData;

	class NodeContent : public Serializable, private noncopyable, private nonmovable
	{
	public:
		NodeContent() = delete;
		~NodeContent();

#ifdef _SWIG_PY
		bool operator==(const NodeContent& other)
		{
			return (this == &other);
		}
#endif

		const std::string& getName() { return mName; }
		const std::string& getPath() { return mPath; }
		const std::string& getLabel() { return mLabel; }

		const std::string& getName() const { return mName; }
		const std::string& getPath() const { return mPath; }
		const std::string& getLabel() const { return mLabel; }

		void setName(const std::string& name);
		void setLabel(const std::string& label) { mLabel = label; }

		template<class TNodeContentData>
		TNodeContentData* createData()
		{
			mem::safeDelete(&mpData);
			mpData = new TNodeContentData();
			return getData<TNodeContentData>();
		}

		template<class TNodeContentData>
		TNodeContentData* getData()
		{
			return static_cast<TNodeContentData*>(mpData);
		}

		NodeContentData::Type getDataType()
		{
			return (mpData != nullptr) ? mpData->getType() : NodeContentData::Type::Invalid;
		}

		int calcSize();
		uint8_t* serialize(uint8_t* buffer);
		uint8_t* deserialize(uint8_t* buffer);
		void updatePath();

	RTE_INTERNAL:
		explicit NodeContent(Node* pOwnerNode);

	private:
		Node* mpOwnerNode;

		std::string mName;
		std::string mPath;

		std::string mLabel;

		NodeContentData* mpData;
	};

}// namespace rte
