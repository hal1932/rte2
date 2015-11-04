#pragma once
#include "common.h"

namespace rte
{
	class TcpReceivedData
	{
	public:
		int clientId;
		uint8_t* buffer;
		int bufferSize;

		TcpReceivedData() = default;
		~TcpReceivedData() = default;

		TcpReceivedData clone()
		{
			TcpReceivedData result;
			result.clientId = clientId;
			result.buffer = new uint8_t[bufferSize];
			memcpy(result.buffer, buffer, bufferSize);
			result.bufferSize = bufferSize;
			return std::move(result);
		}

		TcpReceivedData move()
		{
			TcpReceivedData result;
			result.clientId = clientId;
			result.buffer = buffer;
			result.bufferSize = bufferSize;

			clientId = 0;
			buffer = nullptr;
			bufferSize = 0;

			return std::move(result);
		}

		void destroy()
		{
			clientId = 0;
			mem::safeDelete(&buffer);
			bufferSize = 0;
		}
	};

	class TcpSentData
	{
	public:
		int clientId;
		uint8_t* buffer;
		int bufferSize;
		int sentSize;

		TcpSentData() = default;
		~TcpSentData() = default;

		TcpSentData clone()
		{
			TcpSentData result;
			result.clientId = clientId;
			result.buffer = new uint8_t[bufferSize];
			memcpy(result.buffer, buffer, bufferSize);
			result.bufferSize = bufferSize;
			result.sentSize = sentSize;
			return std::move(result);
		}

		TcpSentData move()
		{
			TcpSentData result;
			result.clientId = clientId;
			result.buffer = buffer;
			result.bufferSize = bufferSize;
			result.sentSize = sentSize;

			clientId = 0;
			buffer = nullptr;
			bufferSize = 0;
			sentSize = 0;

			return std::move(result);
		}

		void destroy()
		{
			clientId = 0;
			mem::safeDelete(&buffer);
			bufferSize = 0;
			sentSize = 0;
		}
	};
}// namespace rte
