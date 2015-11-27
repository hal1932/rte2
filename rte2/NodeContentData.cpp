#include "NodeContentData.h"
#include "memutil.h"

namespace rte
{

	int Int32Data::calcSize()
	{
		return calcSizeInt32(Value);
	}

	uint8_t* Int32Data::serialize(uint8_t* buffer)
	{
		auto ptr = buffer;
		ptr = writeInt32(Value, ptr);
		assert(ptr == buffer + calcSize());
		return ptr;
	}
	
	uint8_t* Int32Data::deserialize(uint8_t* buffer)
	{
		auto ptr = buffer;
		ptr = readInt32(&Value, ptr);
		assert(ptr == buffer + calcSize());
		return ptr;
	}

}// namespace rte
