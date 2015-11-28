#include "NodeSerializationContext.h"

namespace rte {

	NodeDeserializationContext::NodeDeserializationContext(uint8_t* buffer, int bufferSize)
		: mpBuffer(buffer), mpCurrent(buffer), mBufferSize(bufferSize)
	{}

	Node* NodeDeserializationContext::getNext()
	{
		if (!hasNext())
		{
			return nullptr;
		}

		auto pNode = new rte::Node();
		mpCurrent = pNode->deserialize(mpCurrent);
		return pNode;
	}

}// namespace rte
