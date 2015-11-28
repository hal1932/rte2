#pragma once
#include "common.h"
#include "Node.h"

namespace rte {

	class NodeDeserializationContext : private noncopyable, private nonmovable
	{
	public:
		NodeDeserializationContext(uint8_t* buffer, int bufferSize);
		~NodeDeserializationContext() = default;

		bool hasNext() { return (mpCurrent != mpBuffer + mBufferSize); }
		Node* getNext();

	private:
		uint8_t* mpBuffer;
		uint8_t* mpCurrent;
		int mBufferSize;
	};

}// namespace rte
