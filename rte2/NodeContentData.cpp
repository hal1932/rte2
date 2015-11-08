#include "NodeContentData.h"

namespace rte
{

	int Int32Data::calcSize()
	{
		return 0;
	}

	uint8_t* Int32Data::serialize(uint8_t* buffer)
	{
		return buffer;
	}
	
	uint8_t* Int32Data::deserialize(uint8_t* buffer)
	{
		return buffer;
	}

}// namespace rte
