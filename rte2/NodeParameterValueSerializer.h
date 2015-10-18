#pragma once
#include "common.h"

namespace rte
	{

	class NodeParameter;

	class NodeParameterValueSerializer
	{
	public:
		static uint8_t* serialize(uint8_t* ptr, NodeParameter* pParam);
		static uint8_t* deserialize(uint8_t* ptr, NodeParameter* pParam);
	};

}// namespace rte
