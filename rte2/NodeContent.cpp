#include "NodeContent.h"
#include "Node.h"
#include "memutil.h"

namespace rte
{

	NodeContent::NodeContent(Node* pOwnerNode)
		: mpOwnerNode(pOwnerNode),
		  mName(std::to_string(math::xor128())),
		  mpData(nullptr)
	{
		updatePath();
	}

	NodeContent::~NodeContent()
	{
		mem::safeDelete(&mpData);
	}

	void NodeContent::setName(const std::string& name)
	{
		mName = name;
		updatePath();
	}

	int NodeContent::calcSize()
	{
		int size = 0;

		// 名前、ラベル
		size += calcSizeString(mName);
		size += calcSizeString(mLabel);

		// ContentData
		if (mpData != nullptr)
		{
			size += mpData->calcSize();
		}

		return size;
	}

	uint8_t* NodeContent::serialize(uint8_t* buffer)
	{
		auto ptr = buffer;

		// 名前、ラベル
		ptr = writeString(mName, ptr);
		ptr = writeString(mLabel, ptr);

		// ContentData
		if (mpData != nullptr)
		{
			auto dataSize = static_cast<int32_t>(mpData->calcSize());
			ptr = writeInt32(dataSize, ptr);

			auto dataType = static_cast<int32_t>(mpData->getType());
			ptr = writeInt32(dataType, ptr);

			ptr = mpData->serialize(ptr);
		}
		else
		{
			ptr = writeInt32(0, ptr);
		}

		return ptr;
	}

	uint8_t* NodeContent::deserialize(uint8_t* buffer)
	{
		auto ptr = buffer;

		// 名前、ラベル
		ptr = readString(&mName, ptr);
		ptr = readString(&mLabel, ptr);
		updatePath();

		// ContentData
		int32_t dataSize = 0;
		ptr = readInt32(&dataSize, ptr);
		if (dataSize > 0)
		{
			int32_t dataType = 0;
			ptr = readInt32(&dataType, ptr);

			switch (static_cast<NodeContentData::Type>(dataType))
			{
			case NodeContentData::Type::Int32:
				ptr = createData<Int32Data>()->deserialize(ptr);
				break;

			default:
				assert(!"invalid data type");
			}
		}

		return ptr;
	}

	void NodeContent::updatePath()
	{
		mPath = path::combine(mpOwnerNode->getPath(), mName);
	}

}// namespace rte
