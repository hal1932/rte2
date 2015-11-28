#pragma once
#include "common.h"
#include "Socket.h"

namespace rte {
	namespace socketUtil {

		/// 受信キューが空になるまで受信
		int receive(mem::Array<uint8_t>* pReceivedData, Socket* pSocket);

		bool sendReceivedConfirmation(Socket* pSocket);
		bool receiveReceivedConfirmation(Socket* pSocket);

		bool sendKeepAlive(Socket* pSocket);
		bool isKeepAlive(const mem::Array<uint8_t>& data);
		bool replyKeepAlive(Socket* pSocket);

		inline void handleWsaError(const char* msg = nullptr, int err = 0xFFFFFFFF)
		{
			if (err == 0xFFFFFFFF)
			{
				err = WSAGetLastError();
			}
			if (err == NO_ERROR)
			{
				return;
			}
			std::string s(msg);
			if (s.length() > 0) s += ": ";
			logError(s + rte::log::getLastErrorString(err));
		}

	}// namespace socketUtil
}// namespace rte
