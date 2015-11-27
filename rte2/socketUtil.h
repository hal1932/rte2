#pragma once
#include "common.h"
#include "Socket.h"

namespace rte {
	namespace socketUtil {

		/// 受信キューが空になるまで受信
		inline bool receive(mem::Array<uint8_t>* pReceivedData, Socket* pSocket)
		{
			if (pSocket->getAvailabieSize() == 0)
			{
				return false;
			}

			const int cBufferSize = 1024;
			pReceivedData->resize(cBufferSize);

			auto recvBytes = pSocket->recv(pReceivedData->get(), pReceivedData->size());
			if (recvBytes <= 0)
			{
				pReceivedData->invalidate();
				return false;
			}

			pReceivedData->resize(recvBytes);

			while (pSocket->getAvailabieSize() > 0)
			{
				mem::Array<uint8_t> tmp(cBufferSize);
				recvBytes = pSocket->recv(tmp.get(), tmp.size());
				if (recvBytes > 0)
				{
					tmp.resize(recvBytes);
					pReceivedData->append(std::move(tmp));
				}
			}

			return true;
		}

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
