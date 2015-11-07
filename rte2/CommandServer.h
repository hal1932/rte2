#pragma once
#include "common.h"
#include "TcpServer.h"

namespace rte
{
	class Command;

	class CommandServer
	{
	public:
		CommandServer();
		~CommandServer();

		void pushCommand(Command&& pCommand);
	};

}// namespace rte
