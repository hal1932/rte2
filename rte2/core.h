#pragma once

namespace rte {
	// SWIG が namespace のネストに対応してない
	//namespace core {

	class core
	{
	public:
		static bool setup();
		static void shutdown();
	};

	//}
}