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

		~TcpReceivedData()
		{
			if (mAutoDelete)
			{
				deallocate();
			}
		}

		void setAutoDelete(bool enabled)
		{
			mAutoDelete = enabled;
		}

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

		void deallocate()
		{
			clientId = 0;
			mem::safeDelete(&buffer);
			bufferSize = 0;
		}

	private:
		bool mAutoDelete;
	};

	class TcpSentData
	{
	public:
		int clientId;
		uint8_t* buffer;
		int bufferSize;
		int sentSize;

		TcpSentData() = default;

		~TcpSentData()
		{
			if (mAutoDelete)
			{
				deallocate();
			}
		}

		void setAutoDelete(bool enabled)
		{
			mAutoDelete = enabled;
		}

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

		void deallocate()
		{
			clientId = 0;
			mem::safeDelete(&buffer);
			bufferSize = 0;
			sentSize = 0;
		}

		std::string toString()
		{
			return
				std::to_string(clientId) + ", " +
				std::to_string(reinterpret_cast<uintptr_t>(buffer)) + ", " + 
				std::to_string(bufferSize) + ", " +
				std::to_string(sentSize);
		}

	private:
		bool mAutoDelete;
	};

	class Socket;

	bool sendDataToSocket(Socket* pSocket, TcpSentData* pData);
	bool receiveDataFromSocket(TcpReceivedData* pOut, Socket* pSocket);

}// namespace rte
