#include "core.h"
#include "Socket.h"

namespace rte {
	//namespace core
	//{

		//void setup()
		bool core::setup()
		{
			auto result = true;

			result &= Socket::setup();

			return result;
		}

		//void shutdown()
		void core::shutdown()
		{
			Socket::shutdown();
		}

	//}
}