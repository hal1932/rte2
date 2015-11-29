#include "NodeSerializationContext.h"
#include "Node.h"
#include "NodeContent.h"

namespace rte {

	NodeSerializationContext::NodeSerializationContext()
		: mBuffer(true)
	{}

	const mem::Array<uint8_t>& NodeSerializationContext::serialize()
	{
		auto size = 0;
		for (auto pNode : mNodePtrList)
		{
			size += pNode->calcSize();
		}
		for (auto pContent : mContentPtrList)
		{
			size += pContent->calcSize();
		}

		mBuffer.resize(size);

		auto ptr = mBuffer.get();
		for (auto pNode : mNodePtrList)
		{
			ptr = pNode->serialize(ptr);
		}
		for (auto pContent : mContentPtrList)
		{
			ptr = pContent->serialize(ptr);
		}

		return mBuffer;
	}

	/*------------------------------------------------------------*/

	NodeDeserializationContext::NodeDeserializationContext(uint8_t* buffer, int bufferSize)
		: mpBuffer(buffer), mBufferSize(bufferSize)
	{}

	void NodeDeserializationContext::deserialize()
	{
		auto ptr = mpBuffer;

		Node* pNode;
		NodeContent* pContent;

		while (ptr != mpBuffer + mBufferSize)
		{
			auto id = *ptr;
			switch (id)
			{
			case 'n':
				pNode = new Node();
				ptr = pNode->deserialize(ptr);
				mNodePtrList.push_back(pNode);
				break;

			case 'c':
				pContent = new NodeContent();
				ptr = pContent->deserialize(ptr);
				mContentPtrList.push_back(pContent);
				break;
			}
		}
	}

}// namespace rte
